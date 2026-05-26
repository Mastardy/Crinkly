#pragma once

#include "Utility/Types.hpp"

class Bus;

class Timer
{
public:
    Timer();
    
    void Tick(U8 tCycles);
    void Reset();

    U8 Read(Address addr);
    void Write(Address addr, U8 value);

private:
    U16 m_InternalCounter;
    U8 m_TimerCounter;
    U8 m_TimerModulo;
    U8 m_TimerControl;
};
