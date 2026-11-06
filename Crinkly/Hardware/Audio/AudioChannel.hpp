#pragma once
#include "Bus.hpp"

class AudioChannel
{
protected:
    const float DUTIES[4] = {0.125f, 0.25f, 0.5f, 0.75f};
    
public:
    AudioChannel(Address reg0, Bus& bus);
    virtual ~AudioChannel() = default;

    bool DacOn();
    void Disable();
    void Trigger();
    
    virtual void TriggerExtra() = 0;
    virtual void Step(U8 tCycles) = 0;
    virtual float PullSample() = 0;

protected:
    Address m_NRx0;
    Address m_NRx1;
    Address m_NRx2;
    Address m_NRx3;
    Address m_NRx4;

    bool m_IsEnabled;
    Byte m_StatusBit;

    Bus& m_Bus;
};
