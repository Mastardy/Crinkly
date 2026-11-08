#include "AudioChannel.hpp"

AudioChannel::AudioChannel(Address reg0, Bus& bus) : m_Bus(bus)
{
    m_NRx0 = reg0;
    m_NRx1 = reg0 + 1;
    m_NRx2 = reg0 + 2;
    m_NRx3 = reg0 + 3;
    m_NRx4 = reg0 + 4;

    m_IsEnabled = false;
    m_StatusBit = static_cast<Byte>(1 << ((m_NRx0 - 0xFF10) / 5));
    m_Length = 0;
    m_MaxLength = m_NRx0 == 0xFF1A ? 256 : 64;
}

bool AudioChannel::DacOn()
{
    return (m_Bus.ReadRaw(m_NRx2) & 0xF8) != 0;
}

void AudioChannel::Disable()
{
    m_IsEnabled = false;
    m_Bus.WriteRaw(0xFF26, m_Bus.ReadRaw(0xFF26) & ~m_StatusBit);
}

void AudioChannel::PowerOff()
{
    m_IsEnabled = false;
    m_LengthEnabled = false;
}

void AudioChannel::Trigger()
{
    const bool wasEnabled = m_IsEnabled;

    if (m_Length == 0)
    {
        m_Length = m_MaxLength;
        if (m_LengthEnabled && m_NextStepSkipsLength) m_Length--;
    }

    m_IsEnabled = true;
    m_Bus.WriteRaw(0xFF26, m_Bus.ReadRaw(0xFF26) | m_StatusBit);
    TriggerExtra(wasEnabled);

    if (!DacOn()) Disable();
}

void AudioChannel::OnWrite(Address address, Byte value)
{
    if (address == m_NRx1)
    {
        m_Length = m_MaxLength - (value & (m_MaxLength - 1));
    }
    else if (address == m_NRx0 || address == m_NRx2)
    {
        if (!DacOn()) Disable();
    }
    else if (address == m_NRx4)
    {
        const bool lengthEnabled = (value & 0x40) != 0;
        const bool triggered = (value & 0x80) != 0;

        if (!m_LengthEnabled && lengthEnabled && m_Length != 0 && m_NextStepSkipsLength)
        {
            if (--m_Length == 0 && !triggered) Disable();
        }

        m_LengthEnabled = lengthEnabled;
        if (triggered) Trigger();
    }
}

void AudioChannel::ClockLength()
{
    if (!m_LengthEnabled || m_Length == 0) return;
    if (--m_Length == 0) Disable();
}
