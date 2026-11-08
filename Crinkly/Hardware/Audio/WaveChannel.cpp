#include "WaveChannel.hpp"

WaveChannel::WaveChannel(Address reg0, Bus& bus) : AudioChannel(reg0, bus)
{
}

int WaveChannel::Period() const
{
    return 2048 - (m_Bus.ReadRaw(m_NRx3) | ((m_Bus.ReadRaw(m_NRx4) & 0x07) << 8));
}

void WaveChannel::TriggerExtra(bool wasEnabled)
{
    if (wasEnabled && m_Timer == 1)
    {
        const auto offset = ((m_Position + 1) >> 1) & 0x0F;
        if (offset < 4)
        {
            m_Bus.WriteRaw(0xFF30, m_Bus.ReadRaw(0xFF30 + offset));
        }
        else
        {
            for (Address i = 0; i < 4; i++) m_Bus.WriteRaw(0xFF30 + i, m_Bus.ReadRaw(0xFF30 + (offset & ~3) + i));
        }
    }

    m_Position = 0;
    m_Timer = Period() + 3;
    m_TickCarry = 0;
    m_JustRead = false;
}

bool WaveChannel::DacOn()
{
    return (m_Bus.ReadRaw(m_NRx0) & 0x80) != 0;
}

int WaveChannel::RamAccess() const
{
    if (!m_IsEnabled) return Bus::WAVE_RAM_NORMAL;
    return m_JustRead ? m_Position / 2 : Bus::WAVE_RAM_BLOCKED;
}

void WaveChannel::Step(U8 tCycles)
{
    if (!m_IsEnabled)
    {
        m_JustRead = false;
        return;
    }
    
    for (m_TickCarry += tCycles; m_TickCarry >= 2; m_TickCarry -= 2)
    {
        m_JustRead = false;
        if (--m_Timer > 0) continue;

        m_Timer = Period();
        m_Position = (m_Position + 1) & 31;
        m_JustRead = true;
    }
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
