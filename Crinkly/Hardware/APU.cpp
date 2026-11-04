#include "APU.hpp"

#include <cmath>
#include <print>

auto squarePhase = 0.0;
auto sinePhase = 0.0;
auto volume = 0.0001f;

APU::APU(Bus& bus) : m_Bus(bus)
{
    m_Device = std::make_unique<ma_device>();
    m_Config = std::make_unique<ma_device_config>(ma_device_type_playback);

    m_Config->playback.format = ma_format_f32;
    m_Config->playback.channels = 1;
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
    auto out = (float*)output;

    for (ma_uint32 i = 0; i < frameCount; i++)
    {
        auto square = (squarePhase < 0.5) ? 1.0f : -1.0f;
        auto sine = (float)std::sin(2.0 * 3.14159 * sinePhase);
        auto squareFreq = 440.0;
        auto sineFreq = 220.0;

        out[i] = square * volume + sine * volume;

        squarePhase += squareFreq / 48000.0;
        if (squarePhase >= 1.0) squarePhase -= 1.0;

        sinePhase += sineFreq / 48000.0;
        if (sinePhase >= 1.0) sinePhase -= 1.0;
    }
}

void APU::Step(U8 tCycles)
{
    auto NR52 = m_Bus.ReadRaw(0xFF26);

    if (m_NR52 == NR52) return;

    std::print("NR52: {:02X} - ", NR52);

    if (NR52 & 0x80) std::print("Audio ON");
    else std::print("Audio OFF");

    std::print(" | CH4 {} | CH3 {} | CH2 {} | CH1 {}\n", NR52 & 0x8 ? "ON" : "OFF", NR52 & 0x4 ? "ON" : "OFF",
               NR52 & 0x2 ? "ON" : "OFF", NR52 & 0x1 ? "ON" : "OFF");

    m_NR52 = NR52;
}
