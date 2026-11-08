#pragma once
#include "AudioChannel.hpp"

class PulseChannel : public AudioChannel
{
public:
    PulseChannel(Address reg0, bool sweepable, Bus& bus);

    ~PulseChannel() override = default;
    void TriggerExtra(bool wasEnabled) override;
    void OnWrite(Address address, Byte value) override;
    void PowerOff() override;

    void ClockSweep();

    void PrepareSamples();
    float PullSample() override;

private:
    int SweepCalc();

    float m_Duty;
    float m_Volume;
    int m_Period;
    double m_Frequency;
    double m_Phase;

    // Sweep stuff
    bool m_Sweepable;
    bool m_SweepEnabled = false;
    bool m_NegateUsed = false;
    int m_SweepTimer = 0;
    int m_Shadow = 0;
};
