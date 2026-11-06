#include "WaveChannel.hpp"

WaveChannel::WaveChannel(Address reg0, Bus& bus) : AudioChannel(reg0, bus)
{
}

void WaveChannel::TriggerExtra()
{
}

bool WaveChannel::DacOn()
{
    return (m_Bus.ReadRaw(m_NRx0) & 0x80) != 0;
}

void WaveChannel::Step(U8 tCycles)
{
    Trigger();
}

void WaveChannel::PrepareSamples()
{
    for (Address i = 0; i < 16; i++) m_WaveRam[i] = m_Bus.ReadRaw(0xFF30 + i);

    m_Shift = SHIFTS[m_IsEnabled ? (m_Bus.ReadRaw(m_NRx2) >> 5) & 0x03 : 4];
    
    const auto period = m_Bus.ReadRaw(m_NRx3) | ((m_Bus.ReadRaw(m_NRx4) & 0x07) << 8);
    m_Frequency = 65536.0 / (2048 - period);
}

float WaveChannel::PullSample()
{
    m_Phase += m_Frequency / 48000.0;
    if (m_Phase >= 1.0) m_Phase -= 1.0;
    if (m_Shift == 4) return 0.0f;
    
    const auto index = static_cast<int>(m_Phase * 32);
    const auto byte = m_WaveRam[index / 2];
    const auto nibble = (index & 1) ? (byte & 0x0F) : (byte >> 4);
    
    return (static_cast<float>(nibble >> m_Shift) / 7.5f - 1.0f) * 0.1f;
}
