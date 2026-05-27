#include "Timer.hpp"

#include <print>

Timer::Timer()
{
    m_InternalCounter = 0xABCC;
    m_TimerCounter = 0x00;
    m_TimerModulo = 0x00;
    m_TimerControl = 0x00;

    m_OverflowPending = false;
    m_OverflowDelay = 4;
}

void Timer::SetRequestTimerInterruptCallback(std::function<void()> callback)
{
    m_RequestTimerInterrupt = callback;
}

void Timer::Tick(U8 tCycles)
{
    auto timeControlBit = TimerControlBit();

    for (U8 i = 0; i < tCycles; i++)
    {
        if (m_OverflowPending && --m_OverflowDelay == 0)
        {
            m_TimerCounter = m_TimerModulo;
            if (m_RequestTimerInterrupt) m_RequestTimerInterrupt();
            m_OverflowPending = false;
        }

        auto old = m_InternalCounter;
        m_InternalCounter++;

        auto oldSig = (old & timeControlBit) && (m_TimerControl & 0x04);
        auto newSig = (m_InternalCounter & timeControlBit) && (m_TimerControl & 0x04);

        if (!oldSig || newSig || ++m_TimerCounter != 0x00) continue;

        m_OverflowPending = true;
        m_OverflowDelay = 4;
    }
}

void Timer::Reset()
{
    auto signal = (m_InternalCounter & TimerControlBit()) && (m_TimerControl & 0x04);
    m_InternalCounter = 0;

    if (!signal || ++m_TimerCounter != 0x00) return;

    m_OverflowPending = true;
    m_OverflowDelay = 4;
}

U8 Timer::Read(Address addr)
{
    switch (addr)
    {
    case 0xFF04: return static_cast<U8>(m_InternalCounter >> 8);
    case 0xFF05: return m_TimerCounter;
    case 0xFF06: return m_TimerModulo;
    case 0xFF07: return m_TimerControl | 0xF8;
    default:
        std::println("Invalid Timer read address: 0x{:04X}", addr);
        return 0x00;
    }
}

void Timer::Write(Address addr, U8 value)
{
    switch (addr)
    {
    case 0xFF04: Reset();
        break;
    case 0xFF05:
        m_TimerCounter = value;
        if (!m_OverflowPending) break;
        m_OverflowPending = false;
        break;
    case 0xFF06: m_TimerModulo = value;
        break;
    case 0xFF07:
        {
            auto oldSignal = (m_InternalCounter & TimerControlBit()) && (m_TimerControl & 0x04);
            m_TimerControl = value;
            auto newSignal = (m_InternalCounter & TimerControlBit()) && (m_TimerControl & 0x04);
            if (!oldSignal || newSignal || ++m_TimerCounter != 0x00) break;
            m_OverflowPending = true;
            m_OverflowDelay = 4;
        }
        break;
    default:
        std::println("Invalid Timer write address: 0x{:04X}", addr);
        break;
    }
}

U16 Timer::TimerControlBit() const
{
    return ClockSelect[m_TimerControl & 0x03];
}
