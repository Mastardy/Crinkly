#include "APU.hpp"

#include <print>

APU::APU(Bus& bus) : m_Bus(bus),
                     m_Ch1(0xFF10, true, bus),
                     m_Ch2(0xFF15, false, bus),
                     m_Ch3(0xFF1A, bus),
                     m_Ch4(0xFF1F, bus)
{
    m_Bus.m_OnWrite = [this](Address a, Byte v) { OnWrite(a, v); };
    m_Bus.m_WaveRamAccess = [this] { return m_Ch3.RamAccess(); };

    m_Device = std::make_unique<ma_device>();
    m_Config = std::make_unique<ma_device_config>(ma_device_type_playback);

    m_Config->playback.format = ma_format_f32;
    m_Config->playback.channels = 2;
    m_Config->sampleRate = 48000;
    m_Config->dataCallback = DataCallback;
    m_Config->pUserData = this;

    if (ma_device_init(nullptr, m_Config.get(), m_Device.get()) != MA_SUCCESS)
    {
        std::println("Failed to open playback device.");
        return;
    }

    ma_device_start(m_Device.get());
}

APU::~APU()
{
    ma_device_uninit(m_Device.get());
}

void APU::DataCallback(ma_device* device, void* output, const void* input, ma_uint32 frameCount)
{
    static_cast<APU*>(device->pUserData)->PullSamples(device, output, input, frameCount);
}

void APU::PullSamples(ma_device* device, void* output, const void* input, ma_uint32 frameCount)
{
    auto out = static_cast<float*>(output);

    m_Ch1.PrepareSamples();
    m_Ch2.PrepareSamples();
    m_Ch3.PrepareSamples();
    m_Ch4.PrepareSamples();

    const Byte NR50 = m_NR50.load();
    const Byte NR51 = m_NR51.load();
    const Byte NR52 = m_NR52.load();

    const auto lVol = static_cast<float>(((NR50 >> 4) & 0x07) + 1) / 8.0f;
    const auto rVol = static_cast<float>(((NR50 >> 0) & 0x07) + 1) / 8.0f;
    const auto isOn = NR52 & 0x80 ? 1.0f : 0.0f;

    for (ma_uint32 i = 0; i < frameCount; i++)
    {
        const auto s1 = m_Ch1.PullSample();
        const auto s2 = m_Ch2.PullSample();
        const auto s3 = m_Ch3.PullSample();
        const auto s4 = m_Ch4.PullSample();

        auto l = 0.0f;
        auto r = 0.0f;

        if (NR51 & 0x10) l += s1;
        if (NR51 & 0x01) r += s1;
        if (NR51 & 0x20) l += s2;
        if (NR51 & 0x02) r += s2;
        if (NR51 & 0x40) l += s3;
        if (NR51 & 0x04) r += s3;
        if (NR51 & 0x80) l += s4;
        if (NR51 & 0x08) r += s4;

        *out++ = l * lVol * 0.25f * isOn;
        *out++ = r * rVol * 0.25f * isOn;
    }
}

void APU::ClearRegisters()
{
    for (Address i = 0xFF10; i < 0xFF26; i++)
    {
        if (i == 0xFF15 || i == 0xFF1F) continue;
        m_Bus.WriteRaw(i, 0x00);
    }
}

void APU::OnWrite(Address address, Byte value)
{
    if (address != 0xFF26)
    {
        for (auto* channel : m_Channels) channel->OnWrite(address, value);
        return;
    }

    const bool on = (value & 0x80) != 0;
    if (on == m_PoweredOn) return;

    m_PoweredOn = on;
    if (on) PowerOn();
    else PowerOff();
}

void APU::PowerOn()
{
    m_FrameStep = 0;
    for (auto* channel : m_Channels) channel->SetNextStepSkipsLength(false);
}

void APU::PowerOff()
{
    for (auto* channel : m_Channels) channel->PowerOff();
    ClearRegisters();
    m_Bus.WriteRaw(0xFF26, 0x00);
}

void APU::ClockFrameSequencer()
{
    const int step = m_FrameStep;
    m_FrameStep = (m_FrameStep + 1) & 7;

    if ((step & 1) == 0)
    {
        for (auto* channel : m_Channels) channel->ClockLength();
    }

    if (step == 2 || step == 6) m_Ch1.ClockSweep();

    const bool nextSkipsLength = (m_FrameStep & 1) != 0;
    for (auto* channel : m_Channels) channel->SetNextStepSkipsLength(nextSkipsLength);
}

void APU::Step(U8 tCycles)
{
    for (auto* channel : m_Channels) channel->Step(tCycles);

    if (m_PoweredOn)
    {
        m_FrameCycles += tCycles;
        while (m_FrameCycles >= 8192)
        {
            m_FrameCycles -= 8192;
            ClockFrameSequencer();
        }
    }

    auto NR50 = m_Bus.ReadRaw(0xFF24);
    auto NR51 = m_Bus.ReadRaw(0xFF25);
    auto NR52 = m_Bus.ReadRaw(0xFF26);

    if (m_NR50.load() != NR50) m_NR50.store(NR50);
    if (m_NR51.load() != NR51) m_NR51.store(NR51);
    if (m_NR52.load() != NR52) m_NR52.store(NR52);
}
