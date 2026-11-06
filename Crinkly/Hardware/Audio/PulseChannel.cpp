#include "PulseChannel.hpp"

PulseChannel::PulseChannel(Address reg0, bool sweepable, Bus& bus) : AudioChannel(reg0, bus)
{
    m_Duty = 0;
    m_Frequency = 0.0;
    m_Period = 0;
    m_Phase = 0.0;
    m_Volume = 0.0f;

    m_Sweepable = sweepable;
    m_SweepTicks = 0;
    m_SweepCycles = 0;
    m_SweepPace = 0;
}

void PulseChannel::TriggerExtra()
{
    if (!m_Sweepable) return;

    m_SweepTicks = 0;
    m_SweepPace = (m_Bus.ReadRaw(m_NRx0) >> 4) & 0x07;
}

void PulseChannel::Step(U8 tCycles)
{
    Trigger();

    if (!m_Sweepable) return;

    m_SweepCycles += tCycles;
    if (m_SweepCycles < 32768) return;
    m_SweepCycles -= 32768;

    auto NRx0 = m_Bus.ReadRaw(m_NRx0);
    auto pace = (NRx0 >> 4) & 0x07;
    auto down = (NRx0 >> 3) & 0x01;
    auto step = NRx0 & 0x07;

    auto NRx3 = m_Bus.ReadRaw(m_NRx3);
    auto NRx4 = m_Bus.ReadRaw(m_NRx4);
    auto period = NRx3 | ((NRx4 & 0x07) << 8);
    auto next = down ? period - (period >> step) : period + (period >> step);

    auto idle = (m_SweepPace == 0);
    if (!idle && ++m_SweepTicks < m_SweepPace) return;
    m_SweepTicks = 0;
    m_SweepPace = pace;

    if (!down && next > 0x7FF)
    {
        Disable();
        return;
    }

    if (idle) return;

    m_Bus.WriteRaw(m_NRx3, static_cast<Byte>(next & 0xFF));
    m_Bus.WriteRaw(m_NRx4, static_cast<Byte>((NRx4 & 0xF8) | (next >> 8)));
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
