#pragma once
#include <memory>

#include "Bus.hpp"
#include "LCD.hpp"

class PPU
{
public:
    PPU(std::shared_ptr<Bus> bus, std::shared_ptr<LCD> lcd);

    void Step(U8 tCycles);

private:
    void EnterVBlank();
    void UpdateSTAT();

    U32 m_DotCounter;
    U8 m_LY;

    std::shared_ptr<Bus> m_Bus;
    std::shared_ptr<LCD> m_LCD;
};
