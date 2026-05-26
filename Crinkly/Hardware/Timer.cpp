#include "Timer.hpp"

#include <print>

Timer::Timer()
{
    m_InternalCounter = 0xABCC;
    m_TimerCounter = 0x00;
    m_TimerModulo = 0x00;
    m_TimerControl = 0x00;
}

void Timer::Tick(U8 tCycles)
{
}

void Timer::Reset()
{
}

U8 Timer::Read(Address addr)
{
    switch (addr)
    {
    case 0xFF04: return static_cast<U8>(m_InternalCounter >> 8);
    case 0xFF05: return m_TimerCounter;
    case 0xFF06: return m_TimerModulo;
    case 0xFF07: return m_TimerControl;
    default:
        std::println("Invalid Timer read address: 0x{:04X}", addr);
        return 0x00;
    }
}

void Timer::Write(Address addr, U8 value)
{
    switch (addr)
    {
    case 0xFF04: m_InternalCounter = 0x0000;
        break;
    case 0xFF05: m_TimerCounter = value;
        break;
    case 0xFF06: m_TimerModulo = value;
        break;
    case 0xFF07: m_TimerControl = value;
        break;
    default:
        std::println("Invalid Timer write address: 0x{:04X}", addr);
        break;
    }
}
