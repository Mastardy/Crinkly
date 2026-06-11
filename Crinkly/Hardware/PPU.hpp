#pragma once
#include <memory>
#include <array>

#include "Bus.hpp"
#include "LCD.hpp"

class PPU
{
    struct Sprite
    {
        U8 x;
        U8 tile;
        U8 attr;
        U8 oam;
    };

    static constexpr U8 SHADES[4] = {255, 170, 85, 0};

public:
    PPU(const std::shared_ptr<Bus>& bus, const std::shared_ptr<LCD>& lcd);

    void Step(U8 tCycles);

private:
    void UpdateSTAT();

    void PutPixel(U8 x, U8 ly, U8 shade);
    void RenderScanline(U8 ly);
    void RenderBackground(U8 ly, std::array<U8, 160>& bg);
    void RenderSprites(U8 ly, const std::array<U8, 160>& bg);

    U32 m_DotCounter;
    U8 m_LY;

    std::array<U8, 69120> m_FrameBuffer;
    U8 m_WindowLine;

    std::weak_ptr<Bus> m_Bus;
    std::weak_ptr<LCD> m_LCD;
};
