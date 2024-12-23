#include "CPU.hpp"

CPU::CPU(const std::shared_ptr<Bus>& bus)
{
    m_Registers[Register8::A] = 0;
    m_Registers[Register8::B] = 0;
    m_Registers[Register8::C] = 0;
    m_Registers[Register8::D] = 0;
    m_Registers[Register8::E] = 0;
    m_Registers[Register8::F] = 0;
    m_Registers[Register8::H] = 0;
    m_Registers[Register8::L] = 0;

    m_SP = 0x0000;
    m_PC = 0x0000;

    m_Bus = bus;
}

U8 CPU::Register(Register8 reg)
{
    return m_Registers[reg];
}

U16 CPU::Register(Register16 reg)
{
    switch (reg)
    {
    case Register16::SP:
        return m_SP;
    case Register16::PC:
        return m_PC;
    case Register16::AF:
        return static_cast<U16>(m_Registers[Register8::A] << 8) | m_Registers[Register8::F];
    case Register16::BC:
        return static_cast<U16>(m_Registers[Register8::B] << 8) | m_Registers[Register8::C];        
    case Register16::DE:
        return static_cast<U16>(m_Registers[Register8::D] << 8) | m_Registers[Register8::E];
    case Register16::HL:
        return static_cast<U16>(m_Registers[Register8::H] << 8) | m_Registers[Register8::L];
    }
    
    return 0;
}

void CPU::Register(Register8 reg, const U8 value)
{
    m_Registers[reg] = value;
}

void CPU::Register(Register16 reg, const U16& value)
{
    switch (reg)
    {
    case Register16::AF:
        m_Registers[Register8::A] = value >> 8;
        m_Registers[Register8::F] = value & 0xFF;
        break;
    case Register16::BC:
        m_Registers[Register8::B] = value >> 8;
        m_Registers[Register8::C] = value & 0xFF;
        break;
    case Register16::DE:
        m_Registers[Register8::D] = value >> 8;
        m_Registers[Register8::E] = value & 0xFF;
        break;
    case Register16::HL:
        m_Registers[Register8::H] = value >> 8;
        m_Registers[Register8::L] = value & 0xFF;
        break;
    case Register16::SP:
        m_SP = value;
        break;
    case Register16::PC:
        m_PC = value;
        break;
    }
}

void CPU::Flag(Flags flag, bool set)
{
    m_Registers[Register8::F] = set ? m_Registers[Register8::F] | static_cast<U8>(flag) : m_Registers[Register8::F] & ~static_cast<U8>(flag);
}

bool CPU::Flag(Flags flag)
{
    return m_Registers[Register8::F] & static_cast<U8>(flag);
}
