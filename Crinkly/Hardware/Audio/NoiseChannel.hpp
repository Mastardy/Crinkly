#pragma once
#include <atomic>

#include "AudioChannel.hpp"

class NoiseChannel : public AudioChannel
{
public:
    NoiseChannel(Address reg0, Bus& bus);

    void TriggerExtra(bool wasEnabled) override;

    void PrepareSamples();
    float PullSample() override;

private:
    void ClockLfsr();

    U16 m_Lfsr;
    bool m_ShortMode;
    float m_Volume;
    double m_Frequency;
    double m_Phase;
    std::atomic<bool> m_ReSeed;
};
