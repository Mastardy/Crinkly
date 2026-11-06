#include "PPU.hpp"
#include <print>

PPU::PPU(Bus& bus, LCD& lcd) : m_Bus(bus), m_LCD(lcd)
{
    m_DotCounter = 0;
    m_LY = 0;
    m_WindowLine = 0;

    m_FrameBuffer.fill(0);
}

void PPU::Step(U8 tCycles)
{
    if (!(m_Bus.ReadRaw(0xFF40) & 0x80)) // LCD disabled
    {
        m_DotCounter = 0;
        m_LY = 0;
        m_WindowLine = 0;
        m_Bus.WriteRaw(0xFF44, 0);
        m_Bus.WriteRaw(0xFF41, m_Bus.ReadRaw(0xFF41) & 0xFC);

        return;
    }

    for (U8 i = 0; i < tCycles; i++)
    {
        m_DotCounter++;

        if (m_DotCounter == 252 && m_LY < 144)
        {
            RenderScanline(m_LY);
        }

        if (m_DotCounter >= 456)
        {
            m_DotCounter = 0;
            m_LY = (m_LY + 1) % 154;
            m_Bus.WriteRaw(0xFF44, m_LY);

            if (m_LY == 144)
            {
                m_LCD.Present(m_FrameBuffer);
                m_WindowLine = 0;
            }
        }

        UpdateSTAT();
    }
}

void PPU::PutPixel(U8 x, U8 ly, U8 shade)
{
    auto offset = (static_cast<U32>(143 - ly) * 160 + x) * 3;
    m_FrameBuffer[offset++] = SHADES[shade];
    m_FrameBuffer[offset++] = SHADES[shade];
    m_FrameBuffer[offset] = SHADES[shade];
}

void PPU::RenderScanline(U8 ly)
{
    std::array<U8, 160> bg;
    RenderBackground(ly, bg);
    RenderSprites(ly, bg);
}

void PPU::RenderBackground(U8 ly, std::array<U8, 160>& bg)
{
    auto lcdc = m_Bus.ReadRaw(0xFF40);
    U16 bgMapBase = (lcdc & 0x08) ? 0x9C00 : 0x9800;
    U16 wMapBase = (lcdc & 0x40) ? 0x9C00 : 0x9800;

    auto scy = m_Bus.ReadRaw(0xFF42);
    auto scx = m_Bus.ReadRaw(0xFF43);
    auto wy = m_Bus.ReadRaw(0xFF4A);
    auto wx = m_Bus.ReadRaw(0xFF4B);
    auto bgp = m_Bus.ReadRaw(0xFF47);

    auto wOnLine = (lcdc & 0x20) && ly >= wy;
    auto wDrawn = false;

    for (U8 x = 0; x < 160; x++)
    {
        if (!(lcdc & 0x01)) // Background Enabled
        {
            bg[x] = 0;
            PutPixel(x, ly, bgp & 0x03);
            continue;
        }

        auto useWindow = wOnLine && (x + 7) >= wx;
        if (useWindow) wDrawn = true;

        U16 mapBase = useWindow ? wMapBase : bgMapBase;
        U8 mapX = static_cast<U8>(useWindow ? x + 7 - wx : x + scx);
        U8 mapY = useWindow ? m_WindowLine : static_cast<U8>(ly + scy);

        U16 mapAddr = mapBase + static_cast<U16>((mapY / 8) * 32 + (mapX / 8));
        U8 tileIndex = m_Bus.ReadRaw(mapAddr);

        auto tileUnsigned = (lcdc & 0x10) != 0;
        U16 tileAddr = static_cast<U16>(tileUnsigned ? 0x8000 : 0x9000);
        tileAddr += (tileUnsigned ? static_cast<U16>(tileIndex) : static_cast<S8>(tileIndex)) * 16;
        tileAddr += static_cast<U16>((mapY % 8) * 2);

        U8 lowByte = m_Bus.ReadRaw(tileAddr);
        U8 highByte = m_Bus.ReadRaw(tileAddr + 1);

        U8 bit = static_cast<U8>(7 - (mapX % 8));
        U8 shadeId = static_cast<U8>((((highByte >> bit) & 0x01) << 1) | ((lowByte >> bit) & 0x01));

        bg[x] = shadeId;
        PutPixel(x, ly, (bgp >> static_cast<U8>(shadeId * 2)) & 0x03);
    }

    if (wDrawn) m_WindowLine++;
}

void PPU::RenderSprites(U8 ly, const std::array<U8, 160>& bg)
{
    auto lcdc = m_Bus.ReadRaw(0xFF40);
    if (!(lcdc & 0x02)) return; // OBJ disabled

    U8 height = (lcdc & 0x04) ? 16 : 8;

    std::array<Sprite, 10> line;
    U8 count = 0;

    for (U8 i = 0; i < 40; i++)
    {
        U16 entry = 0xFE00 + static_cast<U16>(i * 4);
        S32 top = static_cast<S32>(m_Bus.ReadRaw(entry++) - 16);
        if (ly < top || ly >= top + height) continue;

        line[count++] = {m_Bus.ReadRaw(entry++), m_Bus.ReadRaw(entry++), m_Bus.ReadRaw(entry), i};
        if (count >= 10) break;
    }

    for (S32 a = 0; a < count; a++)
    {
        for (S32 b = 0; b < count - 1 - a; b++)
        {
            auto swap = (line[b].x < line[b + 1].x) || (line[b].x == line[b + 1].x && line[b].oam < line[b + 1].oam);
            if (swap)
            {
                auto temp = line[b];
                line[b] = line[b + 1];
                line[b + 1] = temp;
            }
        }
    }

    U8 obp0 = m_Bus.ReadRaw(0xFF48);
    U8 obp1 = m_Bus.ReadRaw(0xFF49);

    for (U8 s = 0; s < count; s++)
    {
        auto& sprite = line[s];
        auto yFlip = sprite.attr & 0x40;
        auto xFlip = sprite.attr & 0x20;
        auto behindBg = sprite.attr & 0x80;
        auto palette = (sprite.attr & 0x10) ? obp1 : obp0;

        U8 row = static_cast<U8>(ly - (static_cast<S32>(m_Bus.ReadRaw(0xFE00 + sprite.oam * 4)) - 16));
        if (yFlip) row = static_cast<U8>(height - 1 - row);

        U8 tile = sprite.tile;
        if (height == 16)
        {
            tile &= 0xFE;
            if (row >= 8) tile |= 0x01;
        }
        U16 tileAddr = 0x8000 + static_cast<U16>(tile) * 16 + static_cast<U16>((row % 8) * 2);
        U8 lowByte = m_Bus.ReadRaw(tileAddr);
        U8 highByte = m_Bus.ReadRaw(tileAddr + 1);

        for (U8 x = 0; x < 8; x++)
        {
            S32 sx = static_cast<S32>(sprite.x) - 8 + x;
            if (sx < 0 || sx >= 160) continue;

            U8 col = xFlip ? x : static_cast<U8>(7 - x);
            U8 shadeId = static_cast<U8>((((highByte >> col) & 0x01) << 1) | ((lowByte >> col) & 0x01));
            if (shadeId == 0) continue;

            if (behindBg && bg[sx] != 0) continue;

            PutPixel(static_cast<U8>(sx), ly, (palette >> static_cast<U8>(shadeId * 2)) & 0x03);
        }
    }
}

void PPU::UpdateSTAT()
{
    auto stat = m_Bus.ReadRaw(0xFF41);
    auto LYC = m_Bus.ReadRaw(0xFF45);

    auto hBlankInterrupt = (stat & 0x08) != 0;
    auto vBlankInterrupt = (stat & 0x10) != 0;
    auto oamInterrupt = (stat & 0x20) != 0;
    auto lycInterrupt = (stat & 0x40) != 0;

    auto lycCoincidence = m_LY == LYC;

    if (m_LY == 144 && m_DotCounter == 0)
    {
        m_Bus.WriteRaw(0xFF0F, m_Bus.ReadRaw(0xFF0F) | 0x01);
    }

    U8 lastFlag = (stat & 0x03);
    U8 flag = (m_LY >= 144) ? 0x01 : (m_DotCounter < 80) ? 0x02 : (m_DotCounter < 252) ? 0x03 : 0x00;

    auto statInterrupt = lycInterrupt && lycCoincidence && m_DotCounter == 0;
    if (flag != lastFlag)
    {
        statInterrupt |= hBlankInterrupt && flag == 0x00;
        statInterrupt |= vBlankInterrupt && flag == 0x01;
        statInterrupt |= oamInterrupt && flag == 0x02;
    }
    if (statInterrupt) m_Bus.WriteRaw(0xFF0F, m_Bus.ReadRaw(0xFF0F) | 0x02);

    stat = (stat & 0xFC) | flag;
    stat = static_cast<U8>((stat & 0xFB) | (lycCoincidence << 2));

    m_Bus.WriteRaw(0xFF41, stat);
}
