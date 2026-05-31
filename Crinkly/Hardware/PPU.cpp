#include "PPU.h"
#include <print>

PPU::PPU(std::shared_ptr<Bus> bus, std::shared_ptr<LCD> lcd)
{
    m_Bus = bus;
    m_LCD = lcd;

    m_DotCounter = 0;
    m_LY = 0;
}

void PPU::Step(U8 tCycles)
{
    m_Bus->Write(0xFF47, 0xE4);

    for (U8 i = 0; i < tCycles; i++)
    {
        if (++m_DotCounter >= 456)
        {
            m_DotCounter = 0;
            m_LY = (m_LY + 1) % 154;
            m_Bus->Write(0xFF44, m_LY);

            if (m_LY == 153) m_LCD->Render();
        }

        UpdateSTAT();
    }
}

void PPU::UpdateSTAT()
{
    auto stat = m_Bus->Read(0xFF41);
    auto LYC = m_Bus->Read(0xFF45);

    auto hBlankInterrupt = (stat & 0x08) != 0;
    auto vBlankInterrupt = (stat & 0x10) != 0;
    auto oamInterrupt = (stat & 0x20) != 0;
    auto lycInterrupt = (stat & 0x40) != 0;

    auto lycCoincidence = m_LY == LYC;

    if (m_LY == 144 && m_DotCounter == 0)
    {
        m_Bus->Write(0xFF0F, m_Bus->Read(0xFF0F) | 0x01);
    }

    U8 modeFlag;

    if (m_LY >= 144) modeFlag = 0x01;
    else if (m_DotCounter < 80) modeFlag = 0x02;
    else if (m_DotCounter < 252) modeFlag = 0x03;
    else modeFlag = 0x00;

    stat = (stat & 0xFC) | modeFlag;
    stat = static_cast<U8>((stat & 0xFB) | (lycCoincidence << 2));

    m_Bus->Write(0xFF41, stat);
}
