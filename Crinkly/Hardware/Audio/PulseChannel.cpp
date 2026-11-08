#include "PulseChannel.hpp"

PulseChannel::PulseChannel(Address reg0, bool sweepable, Bus& bus) : AudioChannel(reg0, bus)
{
    m_Duty = 0;
    m_Frequency = 0.0;
    m_Period = 0;
    m_Phase = 0.0;
    m_Volume = 0.0f;

    m_Sweepable = sweepable;
}

void PulseChannel::TriggerExtra(bool)
{
    if (!m_Sweepable) return;

    const auto NRx0 = m_Bus.ReadRaw(m_NRx0);
    const auto pace = (NRx0 >> 4) & 0x07;
    const auto shift = NRx0 & 0x07;

    m_Shadow = m_Bus.ReadRaw(m_NRx3) | ((m_Bus.ReadRaw(m_NRx4) & 0x07) << 8);
    m_SweepTimer = pace ? pace : 8;
    m_SweepEnabled = pace != 0 || shift != 0;
    m_NegateUsed = false;

    if (shift != 0) SweepCalc();
}

void PulseChannel::OnWrite(Address address, Byte value)
{
    AudioChannel::OnWrite(address, value);

    if (m_Sweepable && address == m_NRx0 && m_NegateUsed && !(value & 0x08)) Disable();
}

void PulseChannel::PowerOff()
{
    AudioChannel::PowerOff();
    m_SweepEnabled = false;
    m_NegateUsed = false;
}

int PulseChannel::SweepCalc()
{
    const auto NRx0 = m_Bus.ReadRaw(m_NRx0);
    const auto negate = (NRx0 & 0x08) != 0;
    const auto delta = m_Shadow >> (NRx0 & 0x07);

    if (negate) m_NegateUsed = true;

    const auto next = negate ? m_Shadow - delta : m_Shadow + delta;
    if (next > 0x7FF) Disable();
    return next;
}

void PulseChannel::ClockSweep()
{
    if (!m_Sweepable || --m_SweepTimer > 0) return;

    const auto NRx0 = m_Bus.ReadRaw(m_NRx0);
    const auto pace = (NRx0 >> 4) & 0x07;
    const auto shift = NRx0 & 0x07;

    m_SweepTimer = pace ? pace : 8;
    if (!m_SweepEnabled || pace == 0) return;

    const auto next = SweepCalc();
    if (next > 0x7FF || shift == 0) return;

    m_Shadow = next;
    m_Bus.WriteRaw(m_NRx3, static_cast<Byte>(next & 0xFF));
    m_Bus.WriteRaw(m_NRx4, static_cast<Byte>((m_Bus.ReadRaw(m_NRx4) & 0xF8) | (next >> 8)));
    SweepCalc();
}

void PulseChannel::PrepareSamples()
{
    auto NRx1 = m_Bus.ReadRaw(m_NRx1);
    auto NRx2 = m_Bus.ReadRaw(m_NRx2);
    auto NRx3 = m_Bus.ReadRaw(m_NRx3);
    auto NRx4 = m_Bus.ReadRaw(m_NRx4);

    m_Duty = DUTIES[NRx1 >> 6];
    m_Volume = m_IsEnabled ? static_cast<float>(NRx2 >> 4) / 15.0f * 0.1f : 0.0f;
    m_Period = NRx3 | ((NRx4 & 0x07) << 8);
    m_Frequency = 131072.0 / (2048 - m_Period);
}

float PulseChannel::PullSample()
{
    auto value = (m_Phase < m_Duty) ? m_Volume : -m_Volume;

    m_Phase += m_Frequency / 48000.0;
    if (m_Phase >= 1.0) m_Phase -= 1.0;

    return value;
}
