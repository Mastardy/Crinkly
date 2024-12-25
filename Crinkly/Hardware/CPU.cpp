#include "CPU.hpp"

#include <iostream>
#include <print>

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
    m_Registers[Register8::F] = set
                                    ? m_Registers[Register8::F] | static_cast<U8>(flag)
                                    : m_Registers[Register8::F] & ~static_cast<U8>(flag);
}

bool CPU::Flag(Flags flag)
{
    return m_Registers[Register8::F] & static_cast<U8>(flag);
}

void CPU::Step()
{
    if (var bus = m_Bus.lock())
    {
        while (true)
        {
            switch (bus->Read(m_PC++))
            {
            case 0x00:
                std::print("NOP\n");
                break;
            case 0x05:
                std::print("DEC B\n");
                Register(Register8::B, Register(Register8::B) - 1);
                Flag(Flags::Z, Register(Register8::B) == 0);
                break;
            case 0x06:
                {
                    Byte byte = bus->Read(m_PC++);
                    std::print("LD B, 0x{:02X}\n", byte);
                    Register(Register8::B, byte);
                }
                break;
            case 0x0D:
                std::print("DEC C\n");
                Register(Register8::C, Register(Register8::C) - 1);
                Flag(Flags::Z, Register(Register8::C) == 0);
                break;
            case 0x0E:
                {
                    Byte byte = bus->Read(m_PC++);
                    std::print("LD C, 0x{:02X}\n", byte);
                    Register(Register8::C, byte);
                }
                break;
            case 0x1F:
                {
                    std::print("RRA\n");
                    U8 value = Register(Register8::A);
                    bool carry = value & 0x01;
                    value = (value >> 1) | (carry ? 0x80 : 0x00);
                    std::print("{:02X} -> {:02X}\n", Register(Register8::A), value);
                    Register(Register8::A, value);

                    Flag(Flags::C, carry);
                    Flag(Flags::Z, value == 0);
                }
                break;
            case 0x20:
                {
                    U8 byte = bus->Read(m_PC++);
                    std::print("JR NZ, 0x{:02X}\n", byte);
                    S8 offset = static_cast<S8>(byte);
                    if (!Flag(Flags::Z))
                    {
                        m_PC += offset;
                    }
                }
                break;
            case 0x21:
                {
                    Word word = bus->Read(m_PC++) | static_cast<Word>(bus->Read(m_PC++) << 8);
                    std::print("LD HL, 0x{:04X}\n", word);
                    Register(Register16::HL, word);
                }
                break;
            case 0x31:
                {
                    Address adr = bus->Read(m_PC++) | static_cast<Address>(bus->Read(m_PC++) << 8);
                    std::print("LD SP, 0x{:04X}\n", adr);
                    m_SP = adr;
                }
                break;
            case 0x32:
                std::print("LD (HL-), A\n");
                bus->Write(Register(Register16::HL), Register(Register8::A));
                Register(Register16::HL, Register(Register16::HL) - 1);
                break;
            case 0x3E:
                {
                    Byte byte = bus->Read(m_PC++);
                    std::print("LD A, 0x{:02X}\n", byte);
                    Register(Register8::A, byte);
                }
                break;
            case 0xAF:
                std::print("XOR A, A\n");
                Register(Register8::A, 0);
                Flag(Flags::Z, true);
                break;
            case 0xC3:
                {
                    Address adr = bus->Read(m_PC++) | static_cast<Address>(bus->Read(m_PC++) << 8);
                    std::print("JP 0x{:04X}\n", adr);
                    m_PC = adr;
                }
                break;
            case 0xCD:
                {
                    Address adr = bus->Read(m_PC++) | static_cast<Address>(bus->Read(m_PC++) << 8);
                    std::print("CALL 0x{:04X}\n", adr);
                    Push(m_PC);
                    m_PC = adr;
                }
                break;
            case 0xE0:
                {
                    Byte byte = bus->Read(m_PC++);
                    Word word = 0xFF00 | byte;
                    std::print("LDH 0x{:04X}, A\n", word);
                    bus->Write(word, Register(Register8::A));
                }
                break;
            case 0xEA:
                {
                    Address adr = bus->Read(m_PC++) | static_cast<Address>(bus->Read(m_PC++) << 8);
                    std::print("LD (0x{:04X}), A\n", adr);
                    bus->Write(adr, Register(Register8::A));
                }
                break;
            case 0xF0:
                {
                    Byte byte = bus->Read(m_PC++);
                    std::print("LDH A, 0xFF{:02X}\n", byte);
                    Register(Register8::A, bus->Read(0xFF00 | byte));
                }
                break;
            case 0xF3:
                std::print("DI\n");
                bus->Write(0xFFFF, 0x00);
                break;
            case 0xFB:
                std::print("EI\n");
                bus->Write(0xFFFF, 0x01);
                break;
            case 0xFE:
                {
                    Byte byte = bus->Read(m_PC++);
                    std::print("CP A,0x{:02X}\n", byte);
                    U8 result = Register(Register8::A) - byte;
                    std::print("A: 0x{:02X} - 0x{:02X} = {:02X}\n", Register(Register8::A), byte, result);
                    Flag(Flags::Z, result == 0);
                    Flag(Flags::N, true);
                    Flag(Flags::H, (Register(Register8::A) & 0x0F) < (byte & 0x0F));
                    Flag(Flags::C, Register(Register8::A) < byte);
                }
                break;
            default:
                std::print("Unknown opcode! 0x{:02X}\n", bus->Read(m_PC - 1));
                exit(1);
            }
        }
    }
    else
    {
        std::cerr << "Bus is not available!";
        exit(1);
    }
}

void CPU::Step(U16 address)
{
    m_PC = address;
    Step();
}

void CPU::Push(U8 value)
{
    if (var bus = m_Bus.lock())
    {
        bus->Write(--m_SP, value);
    }
}

void CPU::Push(U16 value)
{
    Push(static_cast<U8>(value >> 8));
    Push(static_cast<U8>(value & 0xFF));
}

U8 CPU::Pop()
{
    if (var bus = m_Bus.lock())
    {
        return bus->Read(m_SP++);
    }
    exit(1);
}

U16 CPU::Pop16()
{
    U16 value = Pop();
    value |= static_cast<U16>(Pop() << 8);
    return value;
}