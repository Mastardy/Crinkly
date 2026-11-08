#pragma once

#include <memory>

#include "Cartridge.hpp"
#include "Joypad.hpp"
#include "Utility/Types.hpp"
#include "Utility/Utils.hpp"
#include "Hardware/Timer.hpp"

class Bus
{
private:
    static constexpr Size CARTRIDGE_BANK_ROM_SIZE = 16_Kb;
    static constexpr Size VIDEO_RAM_SIZE = 8_Kb;
    static constexpr Size CARTRIDGE_RAM_SIZE = 8_Kb;
    static constexpr Size WORK_RAM_SIZE = 8_Kb;

    static constexpr int WAVE_RAM_NORMAL = -1;
    static constexpr int WAVE_RAM_BLOCKED = -2;

public:
    Bus(Timer& timer, Joypad& joypad);

    void InsertCartridge(const std::string& cartridge);
    std::string CartridgeName() const { return m_CartridgeName; }

    Byte Read(Address address);
    std::vector<Byte> Read(Address start, Size length);

    void Write(Address, Byte);

    Byte ReadRaw(Address address);
    void WriteRaw(Address, Byte);

    std::shared_ptr<Cartridge> m_Cartridge;
    std::function<void(Address, Byte)> m_OnWrite;

    std::function<int()> m_WaveRamAccess;

private:
    std::vector<Byte> m_VideoRAM;
    std::vector<Byte> m_CartridgeRAM;
    std::vector<Byte> m_WorkRAM;
    std::vector<Byte> m_OAM;
    std::vector<Byte> m_IO_Registers;
    std::vector<Byte> m_HighRAM;

    Timer& m_Timer;
    Joypad& m_Joypad;

    Byte m_InterruptEnable;
    std::string m_CartridgeName;
};
