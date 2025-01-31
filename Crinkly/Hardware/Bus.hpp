#pragma once

#include <memory>

#include "Cartridge.hpp"
#include "Utility/Types.hpp"
#include "Utility/Utils.hpp"

class Bus
{
private: // Specifications
    static constexpr Size CARTRIDGE_BANK_ROM_SIZE = 16_Kb;
    static constexpr Size VIDEO_RAM_SIZE = 8_Kb;
    static constexpr Size CARTRIDGE_RAM_SIZE = 8_Kb;
    static constexpr Size WORK_RAM_SIZE = 8_Kb;
    
public:
    Bus();

    void InsertCartridge(const std::string& cartridge);
    std::string CartridgeName() const { return cartridgeName; }
    
    Byte Read(Address address);
    std::vector<Byte> Read(Address start, Size length);

    void Write(Address, Byte);
    
private:
    std::shared_ptr<Cartridge> m_Cartridge;

    std::vector<Byte> m_CartridgeROM_Bank0;
    std::vector<Byte> m_CartridgeROM_Bank1;
    std::vector<Byte> m_VideoRAM;
    std::vector<Byte> m_CartridgeRAM;
    std::vector<Byte> m_WorkRAM;
    std::vector<Byte> m_OAM;
    std::vector<Byte> m_IO_Registers;
    std::vector<Byte> m_HighRAM;
    Byte m_InterruptEnable;

    std::string cartridgeName;
};
