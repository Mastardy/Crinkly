#pragma once

#include <array>
#include <functional>

#include "Utility/Types.hpp"

class Bus;

class Timer
{
public:
    Timer();

    void SetRequestTimerInterruptCallback(std::function<void()> callback);

    void Tick(U8 tCycles);
    void Reset();

    U8 Read(Address addr);
    void Write(Address addr, U8 value);

private:
    static constexpr std::array<U16, 4> ClockSelect = {1 << 9, 1 << 3, 1 << 5, 1 << 7};

    U16 TimerControlBit() const;

    U16 m_InternalCounter;
    U8 m_TimerCounter;
    U8 m_TimerModulo;
    U8 m_TimerControl;

    U8 m_OverflowDelay;
    bool m_OverflowPending;

    std::function<void()> m_RequestTimerInterrupt;
};
