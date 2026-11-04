#include "APU.hpp"

#include <cmath>
#include <print>

auto ch1Phase = 0.0;
auto ch2Phase = 0.0;

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

    auto NR11 = m_Bus.ReadRaw(0xFF11);
    auto NR12 = m_Bus.ReadRaw(0xFF12);
    auto NR13 = m_Bus.ReadRaw(0xFF13);
    auto NR14 = m_Bus.ReadRaw(0xFF14);

    float duties[4] = {0.125f, 0.25f, 0.5f, 0.75f};
    auto duty = duties[NR11 >> 6];

    auto volume = static_cast<float>(NR12 >> 4) / 15.0f * 0.1f;
    if (!m_Ch1On) volume = 0.0f;

    auto period = NR13 | ((NR14 & 7) << 8);
    auto freq = 131072.0 / (2048 - period);
    
    auto NR21 = m_Bus.ReadRaw(0xFF16);
    auto NR22 = m_Bus.ReadRaw(0xFF17);
    auto NR23 = m_Bus.ReadRaw(0xFF18);
    auto NR24 = m_Bus.ReadRaw(0xFF19);
    
    auto duty2 = duties[NR21 >> 6];
    auto volume2 = static_cast<float>(NR22 >> 4) / 15.0f * 0.1f;
    if (!m_Ch2On) volume2 = 0.0f;
    auto period2 = NR23 | ((NR24 & 7) << 8);
    auto freq2 = 131072.0 / (2048 - period2);

    for (ma_uint32 i = 0; i < frameCount; i++)
    {
        auto ch1 = (ch1Phase < duty) ? volume : -volume;
        auto ch2 = (ch2Phase < duty2) ? volume2 : -volume2;
        out[i] = ch1 + ch2;

        ch1Phase += freq / 48000.0;
        ch2Phase += freq2 / 48000.0;
        if (ch1Phase >= 1.0) ch1Phase -= 1.0;
        if (ch2Phase >= 1.0) ch2Phase -= 1.0;
    }
}

void APU::Step(U8 tCycles)
{
    auto NR14 = m_Bus.ReadRaw(0xFF14);
    if (NR14 & 0x80)
    {
        m_Ch1On = true;
        m_SweepTicks = 0;
        m_Bus.WriteRaw(0xFF14, NR14 & 0x7F);
    }
    
    auto NR24 = m_Bus.ReadRaw(0xFF19);
    if (NR24 & 0x80)
    {
        m_Ch2On = true;
        m_Bus.WriteRaw(0xFF19, NR24 & 0x7F);
    }
    
    m_SweepCycles += tCycles;
    if (m_SweepCycles >= 32768)
    {
        m_SweepCycles -= 32768;

        auto NR10 = m_Bus.ReadRaw(0xFF10);
        auto pace = (NR10 >> 4) & 0x07;
        auto down = (NR10 >> 3) & 1;
        auto step = NR10 & 0x07;

        if (pace != 0 && ++m_SweepTicks >= pace)
        {
            auto NR13 = m_Bus.ReadRaw(0xFF13);
            auto period = NR13 | ((NR14 & 7) << 8);

            auto delta = period >> step;
            period = down ? period - delta : period + delta;
            
            if (period > 0x7FF)
            {
                m_Ch1On = false;
            } 
            else
            {
                m_Bus.WriteRaw(0xFF13, period & 0xFF);
                m_Bus.WriteRaw(0xFF14, static_cast<Byte>((NR14 & 0xF8) | (period >> 8)));
            }
        }
    }

    auto NR50 = m_Bus.ReadRaw(0xFF24);
    auto NR51 = m_Bus.ReadRaw(0xFF25);
    auto NR52 = m_Bus.ReadRaw(0xFF26);

    if (m_NR50 != NR50)
    {
        std::print("NR50: {:02X} - ", NR50);

        double leftVolume = (((NR50 & 0x70) >> 4) + 1) * 12.5;
        double rightVolume = ((NR50 & 0x07) + 1) * 12.5;

        std::print(" VIN Left {} |", NR50 & 0x80 ? "ON" : "OFF");
        std::print(" Left Vol {} |", leftVolume);
        std::print(" VIN Right {} |", NR50 & 0x08 ? "ON" : "OFF");
        std::print(" Right Vol {}\n", rightVolume);

        m_NR50 = NR50;
    }

    if (m_NR51 != NR51)
    {
        std::print("NR51: {:02X} - ", NR51);

        std::print(" CH4 [{}] ",
                   (NR51 & 0x88) == 0x88 ? "Center" : NR51 & 0x80 ? "Left" : NR51 & 0x08 ? "Right" : "Ignored");
        std::print("| CH3 [{}] ",
                   (NR51 & 0x44) == 0x44 ? "Center" : NR51 & 0x40 ? "Left" : NR51 & 0x04 ? "Right" : "Ignored");
        std::print("| CH2 [{}] ",
                   (NR51 & 0x22) == 0x22 ? "Center" : NR51 & 0x20 ? "Left" : NR51 & 0x02 ? "Right" : "Ignored");
        std::print("| CH1 [{}]\n",
                   (NR51 & 0x11) == 0x11 ? "Center" : NR51 & 0x10 ? "Left" : NR51 & 0x01 ? "Right" : "Ignored");

        m_NR51 = NR51;
    }

    if (m_NR52 != NR52)
    {
        std::print("NR52: {:02X} - ", NR52);

        if (NR52 & 0x80) std::print("Audio ON");
        else std::print("Audio OFF");

        std::print(" | CH4 {} | CH3 {} | CH2 {} | CH1 {}\n", NR52 & 0x8 ? "ON" : "OFF", NR52 & 0x4 ? "ON" : "OFF",
                   NR52 & 0x2 ? "ON" : "OFF", NR52 & 0x1 ? "ON" : "OFF");

        m_NR52 = NR52;
    }
}
