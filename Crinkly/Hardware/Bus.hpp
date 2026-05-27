#pragma once

#include <memory>

#include "Cartridge.hpp"
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

public:
    Bus(const std::shared_ptr<Timer>& timer);

    void InsertCartridge(const std::string& cartridge);
    std::string CartridgeName() const { return m_CartridgeName; }

    Byte Read(Address address);
    std::vector<Byte> Read(Address start, Size length);

    void Write(Address, Byte);
    
    std::shared_ptr<Cartridge> m_Cartridge;

private:
    std::vector<Byte> m_CartridgeROM_Bank0;
    std::vector<Byte> m_CartridgeROM_Bank1;
    std::vector<Byte> m_VideoRAM;
    std::vector<Byte> m_CartridgeRAM;
    std::vector<Byte> m_WorkRAM;
    std::vector<Byte> m_OAM;
    std::vector<Byte> m_IO_Registers;
    std::vector<Byte> m_HighRAM;

    std::shared_ptr<Timer> m_Timer;
    Byte m_InterruptEnable;
    std::string m_CartridgeName;
};
