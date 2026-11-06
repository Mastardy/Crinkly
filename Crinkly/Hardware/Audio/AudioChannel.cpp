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

void AudioChannel::Trigger()
{
    if (!DacOn()) Disable();

    auto NRx4 = m_Bus.ReadRaw(m_NRx4);
    if (!(NRx4 & 0x80)) return;

    m_Bus.WriteRaw(m_NRx4, NRx4 & 0x7F);
    if (!DacOn()) return;

    m_IsEnabled = true;
    m_Bus.WriteRaw(0xFF26, m_Bus.ReadRaw(0xFF26) | m_StatusBit);

    TriggerExtra();
}
