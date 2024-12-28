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
        if (const var bus = m_Bus.lock())
        {
            return bus->Read(Register(Register16::HL));
        }
        std::print("Cannot read from indirect register\n");
        return 0;
    }
    
    return reg == Register8::Imm8 ? ReadImm8() : m_Registers[reg];
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

U8 CPU::ReadImm8()
{
    if (const var bus = m_Bus.lock())
    {
        return bus->Read(m_PC++);
    }

    return 0x00;
}

U16 CPU::ReadImm16()
{
    if (const var bus = m_Bus.lock())
    {
        return bus->Read(m_PC++) | static_cast<U16>(bus->Read(m_PC++) << 8);
    }

    return 0x0000;
}


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
            const var block = opcode & 0xC0;
            const var params = opcode & 0x3F;

            if (opcode == 0xDB)
            {
                std::print("\nA: 0x{:02X} ", Register(Register8::A));
                std::print("B: 0x{:02X} ", Register(Register8::B));
                std::print("C: 0x{:02X} ", Register(Register8::C));
                std::print("D: 0x{:02X} ", Register(Register8::D));
                std::print("E: 0x{:02X} ", Register(Register8::E));
                std::print("H: 0x{:02X} ", Register(Register8::H));
                std::print("L: 0x{:02X} ", Register(Register8::L));
                std::print("HL: 0x{:02X}", Register(Register8::HL));

                std::println("");

                std::print("Z: {}, ", Flag(Flags::Z));
                std::print("N: {}, ", Flag(Flags::N));
                std::print("H: {}, ", Flag(Flags::H));
                std::println("C: {}\n", Flag(Flags::C));

                continue;
            }

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
                    case 0x10:
                        Stop(); // stop
                        break;
                    case 0x17:
                        RotateLeftAccumulator(); // rla
                        break;
                    case 0x18:
                        JumpRelative(); // jr imm8
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
                        else if (rightParam == 0x2) // ld [r16mem], a
                        {
                            LoadAccumulatorToR16Address(m_R16mem[reg]);
                            break;
                        }
                        else if (rightParam == 0x3) // inc r16
                        {
                            Increment(m_R16[reg]);
                            break;
                        }
                        else if (rightParam == 0x9) // add hl, r16
                        {
                            Add(m_R16[reg]);
                            break;
                        }
                        else if (rightParam == 0xA) // ld a, [r16mem]
                        {
                            LoadR16AddressToAccumulator(m_R16mem[reg]);
                            break;
                        }
                        else if (rightParam == 0xB) // dec r16
                        {
                            Decrement(m_R16[reg]);
                            break;
                        }

                        reg = (params & 0x38) >> 3;
                        rightParam = params & 0x07;

                        if (rightParam == 0x4) // inc r8
                        {
                            Increment(m_R8[reg]);
                            break;
                        }
                        else if (rightParam == 0x5) // dec r8
                        {
                            Decrement(m_R8[reg]);
                            break;
                        }
                        else if (rightParam == 0x6) // ld r8, imm8
                        {
                            LoadImm8ToR8(m_R8[reg]);
                            break;
                        }

                        if ((params & 0x20) == 0x20 && rightParam == 0x0) // jr cond, imm8
                        {
                            JumpRelativeConditional((params >> 3) & 0x03);
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
                        const U8 dest = (params >> 3) & 0x07;
                        const U8 src = params & 0x07;
                        LoadR8ToR8(m_R8[dest], m_R8[src]);
                    }
                    break;
                }
            case 0x80:
                {
                    const Register8 operand = m_R8[params & 0x07];
                    switch ((params & 0x38) >> 3)
                    {
                    case 0x0:
                        Add(operand);
                        break;
                    case 0x1:
                        Adc(operand);
                        break;
                    case 0x2:
                        Sub(operand);
                        break;
                    case 0x3:
                        Sbc(operand);
                        break;
                    case 0x4:
                        And(operand);
                        break;
                    case 0x5:
                        Xor(operand);
                        break;
                    case 0x6:
                        Or(operand);
                        break;
                    case 0x7:
                        Cp(operand);
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
                        Add(Register8::Imm8);
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
                        Adc(Register8::Imm8);
                        break;
                    case 0x16:
                        Sub(Register8::Imm8);
                        std::println("sub a, imm8");
                        break;
                    case 0x19:
                        std::println("reti");
                        break;
                    case 0x1E:
                        Sbc(Register8::Imm8);
                        std::println("sbc a, imm8");
                        break;
                    case 0x20:
                        std::println("ldh [imm8], a");
                        break;
                    case 0x22:
                        std::println("ldh [c], a");
                        break;
                    case 0x26:
                        And(Register8::Imm8);
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
                        Xor(Register8::Imm8);
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
                        Or(Register8::Imm8);
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
                        Cp(Register8::Imm8);
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

template <class... Types>
void CPU::PrintInstruction(const std::format_string<Types...>& text, Types&&... args)
{
#ifdef PRINT_INSTRUCTION
    std::println(text, std::forward<Types>(args)...);
#endif
}

#pragma region Instructions

#pragma region 16-bit Load Instructions

void CPU::LoadFromStackPointer()
{
    PrintInstruction("ld [imm16], sp");

    U16 address = ReadImm16();
    if (const var bus = m_Bus.lock())
    {
        bus->Write(address++, static_cast<U8>(Register(Register16::SP) & 0xFF));
        bus->Write(address, static_cast<U8>(Register(Register16::SP) >> 8));
    }
}

void CPU::LoadImm16ToR16(Register16 reg)
{
    PrintInstruction("ld r16, imm16");

    const U16 data = ReadImm16();
    Register(reg, data);
}

void CPU::LoadAccumulatorToR16Address(Register16 reg)
{
    PrintInstruction("ld [r16mem], a");

    if (const var bus = m_Bus.lock())
    {
        const U8 data = Register(Register8::A);
        const Address address = Register(reg);
        bus->Write(address, data);
    }
}

void CPU::LoadR16AddressToAccumulator(Register16 reg)
{
    PrintInstruction("ld a, [r16mem]");

    if (const var bus = m_Bus.lock())
    {
        const Address address = Register(reg);
        const U8 data = bus->Read(address);
        Register(Register8::A, data);
    }
}

#pragma endregion

#pragma region 8-bit Load Instructions

void CPU::LoadImm8ToR8(Register8 reg)
{
    PrintInstruction("ld r8, imm8");

    const U8 data = ReadImm8();
    Register(reg, data);
}

void CPU::LoadR8ToR8(Register8 left, Register8 right)
{
    PrintInstruction("ld r8, r8");

    if (const var bus = m_Bus.lock())
    {
        Register(left, Register(left) + Register(right));
    }
}


#pragma endregion

#pragma region 8-bit Arithmetic & Logical Instructions

void CPU::Add(Register8 reg)
{
    PrintInstruction("add a, {}", reg == Register8::Imm8 ? "imm8" : "r8");

    const U8 accumulator = Register(Register8::A);
    const U8 value = Register(reg);
    const U16 result = accumulator + value;

    Register(Register8::A, static_cast<U8>(result));

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, (accumulator & 0x0F) + (value & 0x0F) > 0x0F);
    Flag(Flags::C, result > 0xFF);
}

void CPU::Adc(Register8 reg)
{
    PrintInstruction("adc a, r8");

    const bool carry = Flag(Flags::C);
    const U8 accumulator = Register(Register8::A);
    const U8 value = Register(reg);
    const U16 result = accumulator + value + carry;

    Register(Register8::A, static_cast<U8>(result));

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, ((accumulator & 0x0F) + (value & 0x0F) + carry) > 0x0F);
    Flag(Flags::C, result > 0xFF);
}

void CPU::Sub(Register8 reg)
{
    PrintInstruction("sub a, r8");

    const U8 accumulator = Register(Register8::A);
    const U8 value = Register(reg);
    const U16 result = accumulator - value;

    Register(Register8::A, static_cast<U8>(result));

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, true);
    Flag(Flags::H, (accumulator & 0x0F) < (value & 0x0F));
    Flag(Flags::C, accumulator < value);
}

void CPU::Sbc(Register8 reg)
{
    PrintInstruction("sbc a, r8");

    const bool carry = Flag(Flags::C);
    const U8 accumulator = Register(Register8::A);
    const U8 value = Register(reg);
    const U16 result = accumulator - value - carry;

    Register(Register8::A, static_cast<U8>(result));

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, true);
    Flag(Flags::H, (accumulator & 0x0F) < ((value & 0x0F) + carry));
    Flag(Flags::C, accumulator < (value + carry));
}

void CPU::And(Register8 reg)
{
    PrintInstruction("and a, r8");

    const U8 accumulator = Register(Register8::A);
    const U8 value = Register(reg);
    const U8 result = accumulator & value;

    Register(Register8::A, static_cast<U8>(result));

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, true);
    Flag(Flags::C, false);
}

void CPU::Xor(Register8 reg)
{
    PrintInstruction("xor a, r8");

    const U8 accumulator = Register(Register8::A);
    const U8 value = Register(reg);
    const U8 result = accumulator ^ value;

    Register(Register8::A, result);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, false);
}

void CPU::Or(Register8 reg)
{
    PrintInstruction("or a, r8");

    const U8 accumulator = Register(Register8::A);
    const U8 value = Register(reg);
    const U8 result = accumulator | value;

    Register(Register8::A, result);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, false);
}

void CPU::Cp(Register8 reg)
{
    PrintInstruction("cp a, r8");

    const U8 accumulator = Register(Register8::A);
    const U8 value = Register(reg);
    const U8 result = accumulator - value;

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, true);
    Flag(Flags::H, (accumulator & 0x0F) < (value & 0x0F));
    Flag(Flags::C, accumulator < value);
}

void CPU::Increment(Register8 reg)
{
    PrintInstruction("inc r8");

    const U8 value = Register(reg);
    Register(reg, value + 1);

    Flag(Flags::Z, Register(reg) == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, (value & 0xF) == 0xF);
}

void CPU::Decrement(Register8 reg)
{
    PrintInstruction("dec r8");

    const U8 value = Register(reg);
    Register(reg, value - 1);

    Flag(Flags::Z, Register(reg) == 0x00);
    Flag(Flags::N, true);
    Flag(Flags::H, (value & 0x0F) == 0x0);
}

void CPU::DecimalAdjustAccumulator()
{
    PrintInstruction("dda");

    Register(Register8::A, 0x48 + 0x48);

    const U8 lsn = Register(Register8::A) & 0x0F;
    const U8 msn = Register(Register8::A) >> 4;
    bool carry = Flag(Flags::C);
    U8 correction = 0;

    if (Flag(Flags::H) || lsn > 9) correction |= 0x06;
    if (carry || msn > 0x9) correction |= 0x60;
    carry = carry || msn > 0x9;

    Register(Register8::A, Register(Register8::A) + (Flag(Flags::N) ? -correction : correction));

    Flag(Flags::Z, Register(Register8::A) == 0x00);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);
}

void CPU::ComplementAccumulator()
{
    PrintInstruction("cpl");

    Register(Register8::A, ~Register(Register8::A));

    Flag(Flags::N, true);
    Flag(Flags::H, true);
}

void CPU::SetCarryFlag()
{
    PrintInstruction("scf");

    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, true);
}

void CPU::ComplementCarryFlag()
{
    PrintInstruction("ccf");

    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, !Flag(Flags::C));
}

#pragma endregion

#pragma region 16-bit Arithmetic

void CPU::Add(Register16 reg)
{
    PrintInstruction("add hl, r16");

    const U16 hl = Register(Register16::HL);
    const U16 value = Register(reg);
    const U16 half = (hl & 0x0FFF) + (value & 0x0FFF);
    const U32 result = hl + value;

    Register(Register16::HL, static_cast<U16>(result));

    Flag(Flags::N, false);
    Flag(Flags::H, half > 0x0FFF);
    Flag(Flags::C, result > 0xFFFF);
}

void CPU::Increment(Register16 reg)
{
    PrintInstruction("inc r16");

    Register(reg, Register(reg) + 1);
}

void CPU::Decrement(Register16 reg)
{
    PrintInstruction("dec r16");

    Register(reg, Register(reg) - 1);
}


#pragma endregion

#pragma region Rotate, Shift, Bit Instructions

void CPU::RotateLeftCarryAccumulator()
{
    PrintInstruction("rlca");

    const bool carry = Register(Register8::A) & 0x80;
    Register(Register8::A, static_cast<U8>((Register(Register8::A) << 1) | static_cast<U8>(carry)));

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);
}

void CPU::RotateRightCarryAccumulator()
{
    PrintInstruction("rrca");

    const bool carry = Register(Register8::A) & 0x01;
    Register(Register8::A, static_cast<U8>((Register(Register8::A) >> 1) | (carry ? 0x80 : 0x00)));

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);
}

void CPU::RotateLeftAccumulator()
{
    PrintInstruction("rla");

    const bool carry = Flag(Flags::C);
    const bool b7 = Register(Register8::A) & 0x80;

    Register(Register8::A, static_cast<U8>((Register(Register8::A) << 1) | static_cast<U8>(carry)));

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, b7);
}

void CPU::RotateRightAccumulator()
{
    PrintInstruction("rra");

    const bool carry = Flag(Flags::C);
    const bool b0 = Register(Register8::A) & 0x01;

    Register(Register8::A, static_cast<U8>((Register(Register8::A) >> 1) | (carry ? 0x80 : 0x00)));

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, b0);
}

#pragma endregion

#pragma region Control Flow Instructions

void CPU::JumpRelative()
{
    PrintInstruction("jr imm8");

    const S8 offset = ReadImm8();
    m_PC += offset;
}

void CPU::JumpRelativeConditional(U8 cond)
{
    PrintInstruction("jr {}, imm8", cond);

    const S8 offset = ReadImm8();
    if (Condition(cond)) m_PC += offset;
}

#pragma endregion

#pragma region Miscellanious Instructions

void CPU::Stop() const
{
    PrintInstruction("Stop");
}

#pragma endregion

#pragma endregion
