#pragma once
#include <memory>

#include "Bus.hpp"
#include "miniaudio.h"
#include "PulseChannel.hpp"

class APU
{
public:
    APU(Bus& bus);
    ~APU();

    void Step(U8 tCycles);

private:
    static void DataCallback(ma_device* device, void* output, const void* input, ma_uint32 frameCount);
    void PullSamples(ma_device* device, void* output, const void* input, ma_uint32 frameCount);
    void ClearRegisters();

    std::unique_ptr<ma_device> m_Device;
    std::unique_ptr<ma_device_config> m_Config;
    Bus& m_Bus;

    PulseChannel m_Ch1;
    PulseChannel m_Ch2;

    std::atomic<Byte> m_NR50 = 0;
    std::atomic<Byte> m_NR51 = 0;
    std::atomic<Byte> m_NR52 = 0;
};
