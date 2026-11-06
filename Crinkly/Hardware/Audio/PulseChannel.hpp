#pragma once
#include "AudioChannel.hpp"

class PulseChannel : public AudioChannel
{
public:
    PulseChannel(Address reg0, bool sweepable, Bus& bus);

    ~PulseChannel() override = default;
    void TriggerExtra() override;

    void PrepareSamples();
    void Step(U8 tCycles) override;
    float PullSample() override;

private:
    float m_Duty;
    float m_Volume;
    int m_Period;
    double m_Frequency;
    double m_Phase;
    
    // Sweep stuff
    bool m_Sweepable;
    int m_SweepTicks;
    int m_SweepCycles;
    int m_SweepPace;
};
