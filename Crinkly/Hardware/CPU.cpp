#include "CPU.hpp"

#include <iostream>
#include <print>

#define PRINT_INSTRUCTION

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
    m_Bus = bus;

    m_R8[0] = Register8::B;
    m_R8[1] = Register8::C;
    m_R8[2] = Register8::D;
    m_R8[3] = Register8::E;
    m_R8[4] = Register8::H;
    m_R8[5] = Register8::L;
    m_R8[6] = Register8::HL;
    m_R8[7] = Register8::A;

    m_R16[0] = Register16::BC;
    m_R16[1] = Register16::DE;
    m_R16[2] = Register16::HL;
    m_R16[3] = Register16::SP;

    m_R16stk[0] = Register16::BC;
    m_R16stk[1] = Register16::DE;
    m_R16stk[2] = Register16::HL;
    m_R16stk[3] = Register16::AF;

    m_R16mem[0] = Register16::BC;
    m_R16mem[1] = Register16::DE;
    m_R16mem[2] = Register16::HLi;
    m_R16mem[3] = Register16::HLd;

    m_SP = 0x0000;
    m_PC = 0x0000;
}

#pragma region CPU Registers
U8 CPU::Register(Register8 reg)
{
    if (reg == Register8::HL)
    {
        std::print("Cannot read from indirect register\n");
        return 0;
    }
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
    case Register16::HLi:
    case Register16::HLd:
        std::print("Cannot read from indirect register\n");
        break;
    }

    return 0;
}

void CPU::Register(Register8 reg, const U8 value)
{
    if (reg == Register8::HL)
    {
        std::print("Cannot write to indirect register\n");
        return;
    }
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
    case Register16::HLi:
    case Register16::HLd:
        std::print("Cannot write to indirect register\n");
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
#pragma endregion

#pragma region CPU Stack
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
#pragma endregion

bool CPU::Condition(U8 condition)
{
    switch (condition)
    {
    case 0x00: return !Flag(Flags::Z);
    case 0x01: return Flag(Flags::Z);
    case 0x02: return !Flag(Flags::C);
    case 0x03: return Flag(Flags::C);
    }

    std::print("Unknown condition: 0x{:02X}\n", condition);
    return false;
}

void CPU::Step()
{
    if (var bus = m_Bus.lock())
    {
        while (m_PC < 0x12A)
        {
            const var opcode = bus->Read(m_PC++);
            // const var opcode = 0b0000'1000;
            const var block = opcode & 0xC0;
            const var params = opcode & 0x3F;

            switch (block)
            {
            case 0x00:
                {
                    switch (params)
                    {
                    case 0x00:
                        // std::print("nop\n");
                        break;
                    case 0x07:
                        RotateLeftCarryAccumulator(); // rlca
                        break;
                    case 0x08:
                        LoadFromStackPointer(); // ld [imm16], sp
                        break;
                    case 0x0F:
                        RotateRightCarryAccumulator(); // rrca
                        break;
                    case 0x17:
                        RotateLeftAccumulator(); // rla
                        break;
                    case 0x1F:
                        RotateRightAccumulator(); // rra
                        break;
                    case 0x27:
                        DecimalAdjustAccumulator(); // daa 
                        break;
                    case 0x2F:
                        ComplementAccumulator(); // cpl
                        break;
                    case 0x37:
                        SetCarryFlag(); // scf
                        break;
                    case 0x3F:
                        ComplementCarryFlag(); // ccf
                        break;
                    default:
                        U8 reg = (params & 0x30) >> 4;
                        U8 rightParam = params & 0x0F;

                        if (rightParam == 0x1) // ld r16, imm16
                        {
                            LoadImm16ToR16(m_R16[reg]);
                            break;
                        }
                        else if (rightParam == 0x2) // ld [r16], a
                        {
                            std::print("ld [{}], a\n", static_cast<U8>(m_R16mem[reg]));
                            break;
                        }
                        else if (rightParam == 0x3) // inc r16
                        {
                            std::print("inc {}\n", static_cast<U8>(m_R16[reg]));
                            break;
                        }
                        else if (rightParam == 0x9) // add hl, r16
                        {
                            std::print("add hl, {}\n", static_cast<U8>(m_R16[reg]));
                            break;
                        }
                        else if (rightParam == 0xA) // ld a, [r16]
                        {
                            std::print("ld a, [{}]\n", static_cast<U8>(m_R16mem[reg]));
                            break;
                        }
                        else if (rightParam == 0xB) // dec r16
                        {
                            std::print("dec {}\n", static_cast<U8>(m_R16[reg]));
                            break;
                        }

                        reg = (params & 0x38) >> 3;
                        rightParam = params & 0x07;

                        if (rightParam == 0x4) // inc r8
                        {
                            std::print("inc {}\n", static_cast<U8>(m_R8[reg]));
                            break;
                        }
                        else if (rightParam == 0x5) // dec r8
                        {
                            std::print("dec {}\n", static_cast<U8>(m_R8[reg]));
                            break;
                        }
                        else if (rightParam == 0x6) // ld r8, imm8
                        {
                            std::print("ld {}, imm8\n", static_cast<U8>(m_R8[reg]));
                            break;
                        }

                        if ((params & 0x20) == 0x20 && rightParam == 0x0) // jr cond, imm8
                        {
                            var cond = (params & 0x18) >> 3;
                            std::print("jr {}, imm8\n", cond);
                            break;
                        }
                    }
                    break;
                }
            case 0x40: // Block 1: 8-bit Register-To-Register loads
                {
                    if (params == 0x36)
                    {
                        std::print("halt\n");
                    }
                    else
                    {
                        const U8 dest = (params & 0x38) >> 3;
                        const U8 src = params & 0x07;

                        std::print("ld {}, {}\n", static_cast<U8>(m_R8[dest]), static_cast<U8>(m_R8[src]));
                    }
                    break;
                }
            case 0x80:
                {
                    const U8 operand = static_cast<U8>(m_R8[params & 0x07]);
                    switch ((params & 0x38) >> 3)
                    {
                    case 0x0:
                        std::println("add a, {}", operand);
                        break;
                    case 0x1:
                        std::println("adc a, {}", operand);
                        break;
                    case 0x2:
                        std::println("sub a, {}", operand);
                        break;
                    case 0x3:
                        std::println("sbc a, {}", operand);
                        break;
                    case 0x4:
                        std::println("and a, {}", operand);
                        break;
                    case 0x5:
                        std::println("xor a, {}", operand);
                        break;
                    case 0x6:
                        std::println("or a, {}", operand);
                        break;
                    case 0x7:
                        std::println("cp a, {}", operand);
                        break;
                    }
                    break;
                }
            case 0xC0:
                {
                    switch (params)
                    {
                    case 0x03:
                        std::println("jp imm16");
                        break;
                    case 0x06:
                        std::println("adc a, imm8");
                        break;
                    case 0x09:
                        std::println("ret");
                        break;
                    case 0x0B:
                        std::println("prefix");
                        break;
                    case 0x0D:
                        std::println("call imm16");
                        break;
                    case 0x0E:
                        std::println("add a, imm8");
                        break;
                    case 0x16:
                        std::println("sub a, imm8");
                        break;
                    case 0x19:
                        std::println("reti");
                        break;
                    case 0x1E:
                        std::println("sbc a, imm8");
                        break;
                    case 0x20:
                        std::println("ldh [imm8], a");
                        break;
                    case 0x22:
                        std::println("ldh [c], a");
                        break;
                    case 0x26:
                        std::println("and a, imm8");
                        break;
                    case 0x28:
                        std::println("add sp, imm8");
                        break;
                    case 0x29:
                        std::println("jp hl");
                        break;
                    case 0x2A:
                        std::println("ld [imm16], a");
                        break;
                    case 0x2E:
                        std::println("xor a, imm8");
                        break;
                    case 0x30:
                        std::println("ldh a, [imm8]");
                        break;
                    case 0x32:
                        std::println("ldh a, [c]");
                        break;
                    case 0x33:
                        std::println("di");
                        break;
                    case 0x36:
                        std::println("or a, imm8");
                        break;
                    case 0x38:
                        std::println("ld hl, sp + imm8");
                        break;
                    case 0x39:
                        std::println("ld sp, hl");
                        break;
                    case 0x3A:
                        std::println("ld a, [imm16]");
                        break;
                    case 0x3B:
                        std::println("ei");
                        break;
                    case 0x3E:
                        std::println("cp a, imm8");
                        break;
                    default:
                        const U8 right = params & 0x07;
                        const U8 cond = (params & 0x18) >> 3;
                        const U8 tgt3 = (params & 0x38) >> 3;
                        const U8 rstk = static_cast<U8>(m_R16stk[(params & 0x30) >> 4]);

                        switch (right)
                        {
                        case 0x0:
                            std::println("ret {}", cond);
                            break;
                        case 0x1:
                            std::println("pop {}", rstk);
                            break;
                        case 0x2:
                            std::println("jp {}, imm16", cond);
                            break;
                        case 0x4:
                            std::println("call {}, imm16", cond);
                            break;
                        case 0x5:
                            std::println("push {}", rstk);
                            break;
                        case 0x7:
                            std::println("rst {}", tgt3);
                            break;
                        }
                    }
                    break;
                }
            }
            /*
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
                        }*/
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

#pragma region Instructions

#pragma region 16-bit Load Instructions

void CPU::LoadFromStackPointer()
{
#ifdef PRINT_INSTRUCTION
    std::print("ld [imm16], sp\n");
#endif

    if (var bus = m_Bus.lock())
    {
        U16 address = bus->Read(m_PC++) | static_cast<U16>(bus->Read(m_PC++) << 8);
        bus->Write(address++, static_cast<U8>(Register(Register16::SP) & 0xFF));
        bus->Write(address, static_cast<U8>(Register(Register16::SP) >> 8));
    }
}

void CPU::LoadImm16ToR16(Register16 reg)
{
#ifdef PRINT_INSTRUCTION
    std::println("ld r16, imm16");
#endif
    
    if (var bus = m_Bus.lock())
    {
        const U16 data = bus->Read(m_PC++) | static_cast<U16>(bus->Read(m_PC) << 8);
        Register(reg, data);

        std::println("{} - 0x{:04X}", static_cast<U8>(reg), data);
    }
}

#pragma endregion

#pragma region 8-bit Arithmetic & Logical Instructions

void CPU::DecimalAdjustAccumulator()
{
#ifdef PRINT_INSTRUCTION
    std::println("dda");
#endif

    Register(Register8::A, 0x48 + 0x48);

    U8 correction = 0;
    bool carry = Flag(Flags::C);
    const U8 lsn = Register(Register8::A) & 0x0F; // Least Significant Nibble
    const U8 msn = Register(Register8::A) >> 4; // Most Significant Nibble

    if (Flag(Flags::H) || lsn > 9)
    {
        correction |= 0x06;
    }
    if (carry || msn > 0x9)
    {
        correction |= 0x60;
        carry = true;
    }

    Register(Register8::A, Register(Register8::A) + (Flag(Flags::N) ? -correction : correction));
    
    Flag(Flags::Z, Register(Register8::A) == 0x00);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);
}

void CPU::ComplementAccumulator()
{
#ifdef PRINT_INSTRUCTION
    std::println("cpl");
#endif

    Register(Register8::A, ~Register(Register8::A));

    Flag(Flags::N, true);
    Flag(Flags::H, true);
}

void CPU::SetCarryFlag()
{
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, true);
}

void CPU::ComplementCarryFlag()
{
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, !Flag(Flags::C));
}

#pragma endregion

#pragma region Rotate, Shift, Bit Instructions

void CPU::RotateLeftCarryAccumulator()
{
#ifdef PRINT_INSTRUCTION
    std::print("rlca\n");
#endif

    const bool carry = Register(Register8::A) & 0x80;
    Register(Register8::A, static_cast<U8>((Register(Register8::A) << 1) | carry));

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);
}

void CPU::RotateRightCarryAccumulator()
{
#ifdef PRINT_INSTRUCTION
    std::println("rrca");
#endif

    const bool carry = Register(Register8::A) & 0x01;
    Register(Register8::A, static_cast<U8>((Register(Register8::A) >> 1) | (carry ? 0x80 : 0x00)));

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);
}

void CPU::RotateLeftAccumulator()
{
#ifdef PRINT_INSTRUCTION
    std::println("rla");
#endif

    const bool carry = Flag(Flags::C);
    const bool b7 = Register(Register8::A) & 0x80;

    Register(Register8::A, static_cast<U8>((Register(Register8::A) << 1) | carry));

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, b7);
}

void CPU::RotateRightAccumulator()
{
#ifdef PRINT_INSTRUCTION
    std::println("rra");
#endif

    const bool carry = Flag(Flags::C);
    const bool b0 = Register(Register8::A) & 0x01;

    Register(Register8::A, static_cast<U8>((Register(Register8::A) >> 1) | (carry ? 0x80 : 0x00)));

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, b0);
}

#pragma endregion

#pragma endregion
