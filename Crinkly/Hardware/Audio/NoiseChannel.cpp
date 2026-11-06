#include "NoiseChannel.hpp"

NoiseChannel::NoiseChannel(Address reg0, Bus& bus) : AudioChannel(reg0, bus)
{
    m_Lfsr = 0;
    m_ShortMode = false;
    m_Volume = 0.0f;
    m_Frequency = 0.0;
    m_Phase = 0.0;
    m_ReSeed = false;
}

void NoiseChannel::TriggerExtra()
{
    m_ReSeed = true;
}

void NoiseChannel::Step(U8 tCycles)
{
    Trigger();
}

void NoiseChannel::PrepareSamples()
{
    if (m_ReSeed.exchange(false)) m_Lfsr = 0;

    const auto NRx2 = m_Bus.ReadRaw(m_NRx2);
    const auto NRx3 = m_Bus.ReadRaw(m_NRx3);

    m_Volume = m_IsEnabled ? static_cast<float>(NRx2 >> 4) / 15.0f * 0.1f : 0.0f;
    m_ShortMode = (NRx3 & 0x08) != 0;

    const auto shift = NRx3 >> 4;
    const auto divider = (NRx3 & 0x07) == 0 ? 0.5 : (NRx3 & 0x07);
    m_Frequency = 262144.0 / (divider * (1 << shift));
}

void NoiseChannel::ClockLfsr()
{
    const auto bit = ~(m_Lfsr ^ (m_Lfsr >> 1)) & 1;
    m_Lfsr = static_cast<U16>(((m_Lfsr >> 1) | (bit << 14)));
    if (m_ShortMode) m_Lfsr = static_cast<U16>((m_Lfsr & ~(1 << 6)) | (bit << 6));
}

float NoiseChannel::PullSample()
{
    m_Phase += m_Frequency / 48000.0;
    while (m_Phase >= 1.0)
    {
        m_Phase -= 1.0;
        ClockLfsr();
    }
    
    return (m_Lfsr & 1) ? -m_Volume : m_Volume;
}
