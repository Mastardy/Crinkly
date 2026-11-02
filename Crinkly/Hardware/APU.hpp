#pragma once
#include <memory>

#include "Bus.hpp"
#include "miniaudio.h"

class APU
{
public:
    APU();
    ~APU();

    static void DataCallback(ma_device* device, void* output, const void* input, ma_uint32 frameCount);
    void PullSamples(ma_device* device, void* output, const void* input, ma_uint32 frameCount);
    void Step();

private:
    std::unique_ptr<ma_device> m_Device;
    std::unique_ptr<ma_device_config> m_Config;
    std::weak_ptr<Bus> m_Bus;
};
