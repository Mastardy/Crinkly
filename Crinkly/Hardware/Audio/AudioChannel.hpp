#pragma once
#include "Bus.hpp"

class AudioChannel
{
protected:
    const float DUTIES[4] = {0.125f, 0.25f, 0.5f, 0.75f};

public:
    AudioChannel(Address reg0, Bus& bus);
    virtual ~AudioChannel() = default;

    void Disable();
    void Trigger();
    void ClockLength();

    void OnWrite(Address address, Byte value);

    virtual bool DacOn();

    virtual void TriggerExtra() = 0;
    virtual void Step(U8 tCycles) = 0;
    virtual float PullSample() = 0;

protected:
    Address m_NRx0;
    Address m_NRx1;
    Address m_NRx2;
    Address m_NRx3;
    Address m_NRx4;

    int m_Length;
    int m_MaxLength;
    bool m_IsEnabled;
    Byte m_StatusBit;

    Bus& m_Bus;
};
