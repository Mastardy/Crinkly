#pragma once
#include "AudioChannel.hpp"

class WaveChannel : public AudioChannel
{
    static constexpr int SHIFTS[4] = {4, 0, 1, 2};

public:
    WaveChannel(Address reg0, Bus& bus);

    void TriggerExtra(bool wasEnabled) override;
    bool DacOn() override;

    int RamAccess() const;

    void PrepareSamples();
    void Step(U8 tCycles) override;
    float PullSample() override;

private:
    int Period() const;

    int m_Position = 0;
    int m_Timer = 0;
    int m_TickCarry = 0;
    bool m_JustRead = false;

    Byte m_WaveRam[16] = {};
    int m_Shift = 4;
    double m_Frequency = 0.0;
    double m_Phase = 0.0;
};
