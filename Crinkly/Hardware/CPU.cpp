#include "CPU.hpp"

#include <iostream>
#include <print>
#include <thread>

static std::array<U8, 4> PCMEM;
static std::string str;
static std::string word;

// #define PRINT_INSTRUCTION

static int lineCounter = 0;
static int counter = 0;
static int lastCounter = 0;

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
    m_IME = false;
    m_IME_Next_Cycle = false;
    m_Interrupting = false;
    m_Wait = 0;
}

CPU::~CPU()
{
    m_Log.close();
}

void CPU::Bootstrap()
{
    Register(Register8::A, 0x01);
    Register(Register8::F, 0xB0);
    Register(Register8::B, 0x00);
    Register(Register8::C, 0x13);
    Register(Register8::D, 0x00);
    Register(Register8::E, 0xD8);
    Register(Register8::H, 0x01);
    Register(Register8::L, 0x4D);
    Register(Register16::SP, 0xFFFE);
    Register(Register16::PC, 0x0100);

    if (const auto bus = m_Bus.lock())
    {
        m_Log.open(bus->CartridgeName() + ".log");
    }
}

#pragma region CPU
#pragma region CPU Registers
U8 CPU::Register(Register8 reg)
{
    if (reg == Register8::HL)
    {
        if (const auto bus = m_Bus.lock())
        {
            return bus->Read(Register(Register16::HL));
        }
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
    case Register16::HLi:
    case Register16::HLd:
    case Register16::HL:
        return static_cast<U16>(m_Registers[Register8::H] << 8) | m_Registers[Register8::L];
    case Register16::Imm16:
        return ReadImm16();
    }

    return 0;
}

void CPU::Register(Register8 reg, const U8 value)
{
    if (reg == Register8::HL)
    {
        if (const auto bus = m_Bus.lock())
        {
            bus->Write(Register(Register16::HL), value);
        }
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
        m_Registers[Register8::F] = value & 0xF0;
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
        std::println("Cannot write to indirect register");
        break;
    case Register16::Imm16:
        if (const auto bus = m_Bus.lock())
        {
            const Address address = ReadImm16();
            bus->Write(address, static_cast<U8>(value));
        }
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

#pragma endregion

#pragma region CPU Read
U8 CPU::ReadImm8()
{
    if (const auto bus = m_Bus.lock())
    {
        return bus->Read(m_PC++);
    }

    return 0x00;
}

U16 CPU::ReadImm16()
{
    if (const auto bus = m_Bus.lock())
    {
        return static_cast<U16>(bus->Read(m_PC++)) | static_cast<U16>(bus->Read(m_PC++) << 8);
    }

    return 0x0000;
}
#pragma endregion

#pragma region CPU Stack
void CPU::Push(U8 value)
{
    if (auto bus = m_Bus.lock())
    {
        bus->Write(--m_SP, value);
    }
}

void CPU::Push(U16 value)
{
    Push(static_cast<U8>(value >> 8));
    Push(static_cast<U8>(value & 0xFF));
}

void CPU::Push(Register16 reg)
{
    const U16 value = Register(reg);
    Push(value);
}


U8 CPU::Pop()
{
    if (auto bus = m_Bus.lock())
    {
        return bus->Read(m_SP++);
    }
    exit(1);
}

void CPU::Pop(Register16 reg)
{
    U16 value = Pop();
    value |= static_cast<U16>(Pop() << 8);

    Register(reg, value);
}


U16 CPU::Pop16()
{
    U16 value = Pop();
    value |= static_cast<U16>(Pop() << 8);
    return value;
}
#pragma endregion
#pragma endregion

void CPU::Step()
{
    if (auto bus = m_Bus.lock())
    {
        while (m_PC < 0xFFFF)
        {
            PCMEM[0] = bus->Read(m_PC);
            PCMEM[1] = bus->Read(m_PC + 1);
            PCMEM[2] = bus->Read(m_PC + 2);
            PCMEM[3] = bus->Read(m_PC + 3);

            str = std::format(
                "A:{:02X} F:{:02X} B:{:02X} C:{:02X} D:{:02X} E:{:02X} H:{:02X} L:{:02X} SP:{:04X} PC:{:04X} PCMEM:{:02X},{:02X},{:02X},{:02X}",
                Register(Register8::A), Register(Register8::F), Register(Register8::B), Register(Register8::C),
                Register(Register8::D), Register(Register8::E), Register(Register8::H), Register(Register8::L),
                Register(Register16::SP), Register(Register16::PC), PCMEM[0], PCMEM[1], PCMEM[2], PCMEM[3]);
            m_Log << str << '\n';

            if (++lineCounter % 100000 == 0) std::println("{}", lineCounter);
            
            auto TAC = bus->Read(0xFF07);
            auto TMA = bus->Read(0xFF06);
            auto TIMA = bus->Read(0xFF05);
            
            if (TAC & 0x04)
            {
                ++counter;
                
                if ((TAC & 0x03) == 0x03)
                {
                    if (counter - lastCounter >= 64)
                    {
                        lastCounter = counter;
                        ++TIMA;
                        if (TIMA == 0x00)
                        {
                            bus->Write(0xFF05, TMA);
                            bus->Write(0xFF0F, bus->Read(0xFF0F) | 0x04);
                        }
                        else
                        {
                            bus->Write(0xFF05, TIMA);
                        }
                    }
                }
                else if ((TAC & 0x03) == 0x02)
                {
                    if (counter - lastCounter >= 16)
                    {
                        lastCounter = counter;
                        ++TIMA;
                        if (TIMA == 0x00)
                        {
                            bus->Write(0xFF05, TMA);
                            bus->Write(0xFF0F, bus->Read(0xFF0F) | 0x04);
                        }
                        else
                        {
                            bus->Write(0xFF05, TIMA);
                        }
                    }
                }
                else if ((TAC & 0x03) == 0x01)
                {
                    if (counter - lastCounter >= 4)
                    {
                        lastCounter = counter;
                        ++TIMA;
                        if (TIMA == 0x00)
                        {
                            bus->Write(0xFF05, TMA);
                            bus->Write(0xFF0F, bus->Read(0xFF0F) | 0x04);
                        }
                        else
                        {
                            bus->Write(0xFF05, TIMA);
                        }
                    }
                }
                else
                {
                    if (counter - lastCounter >= 256)
                    {
                        lastCounter = counter;
                        ++TIMA;
                        if (TIMA == 0x00)
                        {
                            bus->Write(0xFF05, TMA);
                            bus->Write(0xFF0F, bus->Read(0xFF0F) | 0x04);
                        }
                        else
                        {
                            bus->Write(0xFF05, TIMA);
                        }
                    }
                }
            }

            if (m_IME_Next_Cycle)
            {
                m_IME = true;
                m_IME_Next_Cycle = false;
            }

            if (m_IME)
            {
                auto interrupt = bus->Read(0xFF0F) & bus->Read(0xFFFF);
                if (interrupt != 0x00)
                {
                    m_IME = false;
                    m_IME_Next_Cycle = false;
                    m_Interrupting = true;

                    Push(m_PC);
                    if (interrupt & 0x01)
                    {
                        bus->Write(0xFF0F, bus->Read(0xFF0F) & ~0x01);
                        m_PC = 0x0040;
                    }
                    else if (interrupt & 0x02)
                    {
                        bus->Write(0xFF0F, bus->Read(0xFF0F) & ~0x02);
                        m_PC = 0x0048;
                    }
                    else if (interrupt & 0x04)
                    {
                        bus->Write(0xFF0F, bus->Read(0xFF0F) & ~0x04);
                        m_PC = 0x0050;
                    }
                    else if (interrupt & 0x08)
                    {
                        bus->Write(0xFF0F, bus->Read(0xFF0F) & ~0x08);
                        m_PC = 0x0058;
                    }
                    else if (interrupt & 0x10)
                    {
                        bus->Write(0xFF0F, bus->Read(0xFF0F) & ~0x10);
                        m_PC = 0x0060;
                    }
                }
            }

            const auto opcode = bus->Read(m_PC++);
            const auto block = opcode & 0xC0;
            const auto params = opcode & 0x3F;

#ifdef PRINT_INSTRUCTION
            std::println(
                "PC: {:04X} OP: {:02X} A: {:02X} B: {:02X} C: {:02X} D: {:02X} E: {:02X} H: {:02X} L: {:02X} Z: {} N: {} H: {} C: {}",
                m_PC - 1, opcode, Register(Register8::A), Register(Register8::B), Register(Register8::C),
                Register(Register8::D), Register(Register8::E), Register(Register8::H), Register(Register8::L),
                Flag(Flags::Z), Flag(Flags::N), Flag(Flags::H), Flag(Flags::C));
#endif

            if (bus->Read(0xFF02) == 0x81)
            {
                char c = static_cast<char>(bus->Read(0xFF01));
                if (c == ' ' || c == '\n') word = "";
                else word += c;
                std::print("{}", c);
                if (word == "Passed" || word == "Failed")
                {
                    std::print("{}", '\n');
                    break;
                }
                bus->Write(0xFF02, 0x0);
            }

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
                    case 0x00: // nop
#ifdef PRINT_INSTRUCTION
                        // std::println("nop");
#endif
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
                        LoadR8ToR8(m_R8[dest], m_R8[src]); // ld r8, r8
                    }
                    break;
                }
            case 0x80:
                {
                    const Register8 operand = m_R8[params & 0x07];
                    switch ((params & 0x38) >> 3)
                    {
                    case 0x0:
                        Add(operand); // add a, r8
                        break;
                    case 0x1:
                        Adc(operand); // adc a, r8
                        break;
                    case 0x2:
                        Sub(operand); // sub a, r8
                        break;
                    case 0x3:
                        Sbc(operand); // sbc a, r8
                        break;
                    case 0x4:
                        And(operand); // and a, r8
                        break;
                    case 0x5:
                        Xor(operand); // xor a, r8
                        break;
                    case 0x6:
                        Or(operand); // or a, r8
                        break;
                    case 0x7:
                        Cp(operand); // cp a, r8
                        break;
                    }
                    break;
                }
            case 0xC0:
                {
                    switch (params)
                    {
                    case 0x03:
                        Jump(Register16::Imm16);
                        break;
                    case 0x06:
                        Add(Register8::Imm8);
                        break;
                    case 0x09:
                        Return();
                        break;
                    case 0x0B:
                        {
                            const U8 suffix = bus->Read(m_PC++);
                            const U8 suffixBlock = (suffix & 0xC0) >> 6;
                            const U8 operand = suffix & 0x7;
                            const U8 bitIndex = (suffix & 0x38) >> 3;

                            switch (suffixBlock)
                            {
                            case 0x0:
                                switch (bitIndex)
                                {
                                case 0x0:
                                    RotateLeftCarry(m_R8[operand]);
                                    break;
                                case 0x1:
                                    RotateRightCarry(m_R8[operand]);
                                    break;
                                case 0x2:
                                    RotateLeft(m_R8[operand]);
                                    break;
                                case 0x3:
                                    RotateRight(m_R8[operand]);
                                    break;
                                case 0x4:
                                    ShiftLeft(m_R8[operand]);
                                    break;
                                case 0x5:
                                    ShiftRight(m_R8[operand]);
                                    break;
                                case 0x6:
                                    Swap(m_R8[operand]);
                                    break;
                                case 0x7:
                                    ShiftRightLogically(m_R8[operand]);
                                    break;
                                }
                                break;
                            case 0x1:
                                Bit(bitIndex, m_R8[operand]);
                                break;
                            case 0x2:
                                Reset(bitIndex, m_R8[operand]);
                                break;
                            case 0x3:
                                Set(bitIndex, m_R8[operand]);
                                break;
                            }
                            break;
                        }
                    case 0x0D:
                        Call();
                        break;
                    case 0x0E:
                        Adc(Register8::Imm8);
                        break;
                    case 0x16:
                        Sub(Register8::Imm8);
                        break;
                    case 0x19:
                        ReturnI();
                        break;
                    case 0x1E:
                        Sbc(Register8::Imm8);
                        break;
                    case 0x20:
                        LoadHighFromAccumulator(Register8::Imm8);
                        break;
                    case 0x22:
                        LoadHighFromAccumulator(Register8::C);
                        break;
                    case 0x26:
                        And(Register8::Imm8);
                        break;
                    case 0x28:
                        AddSP();
                        break;
                    case 0x29:
                        Jump(Register16::HL);
                        break;
                    case 0x2A:
                        LoadAccumulatorToR16Address(Register16::Imm16);
                        break;
                    case 0x2E:
                        Xor(Register8::Imm8);
                        break;
                    case 0x30:
                        LoadHighToAccumulator(Register8::Imm8);
                        break;
                    case 0x32:
                        LoadHighToAccumulator(Register8::C);
                        break;
                    case 0x33:
                        DisableInterrupts();
                        break;
                    case 0x36:
                        Or(Register8::Imm8);
                        break;
                    case 0x38:
                        LoadSPOffsetToHL();
                        break;
                    case 0x39:
                        LoadHLToSP();
                        break;
                    case 0x3A:
                        LoadR16AddressToAccumulator(Register16::Imm16);
                        break;
                    case 0x3B:
                        EnableInterrupts();
                        break;
                    case 0x3E:
                        Cp(Register8::Imm8);
                        break;
                    default:
                        const U8 right = params & 0x07;
                        const U8 cond = (params & 0x18) >> 3;
                        const U8 tgt3 = (params & 0x38) >> 3;
                        const Register16 rstk = m_R16stk[(params & 0x30) >> 4];

                        switch (right)
                        {
                        case 0x0:
                            ReturnConditional(cond);
                            break;
                        case 0x1:
                            Pop(rstk);
                            break;
                        case 0x2:
                            JumpConditional(cond);
                            break;
                        case 0x4:
                            CallConditional(cond);
                            break;
                        case 0x5:
                            Push(rstk);
                            break;
                        case 0x7:
                            Restart(tgt3);
                            break;
                        }
                    }
                    break;
                }
            }

            // std::this_thread::sleep_for(std::chrono::milliseconds(20));
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
    U16 address = ReadImm16();

    PrintInstruction("ld {:04X}, sp[{:04X}]", address, Register(Register16::SP));

    if (const auto bus = m_Bus.lock())
    {
        bus->Write(address++, static_cast<U8>(Register(Register16::SP) & 0xFF));
        bus->Write(address, static_cast<U8>(Register(Register16::SP) >> 8));
    }
}

void CPU::LoadImm16ToR16(Register16 reg)
{
    const U16 data = ReadImm16();
    PrintInstruction("ld {}, imm16", RegisterLiteral(reg), data);
    Register(reg, data);
}

void CPU::LoadAccumulatorToR16Address(Register16 reg)
{
    PrintInstruction("ld [{}], a", RegisterLiteral(reg));

    if (const auto bus = m_Bus.lock())
    {
        const U8 data = Register(Register8::A);
        const Address address = Register(reg);
        bus->Write(address, data);

        if (address == 0xFF0F)
        {
            std::println("Address: {:04X} -> {:02X}", address, data);
        }

        if (reg == Register16::HLd)
        {
            Register(Register16::HL, address - 1);
        }
        else if (reg == Register16::HLi)
        {
            Register(Register16::HL, address + 1);
        }
    }
}

void CPU::LoadR16AddressToAccumulator(Register16 reg)
{
    PrintInstruction("ld a, {}", RegisterLiteral(reg));

    if (const auto bus = m_Bus.lock())
    {
        const Address address = Register(reg);
        const U8 data = bus->Read(address);
        Register(Register8::A, data);

        if (address == 0xFF0F)
        {
            std::println("Address: {:04X} -> {:02X}", address, data);
        }

        if (reg == Register16::HLd)
        {
            Register(Register16::HL, Register(Register16::HL) - 1);
        }
        else if (reg == Register16::HLi)
        {
            Register(Register16::HL, Register(Register16::HL) + 1);
        }
    }
}

void CPU::LoadSPOffsetToHL()
{
    PrintInstruction("ld hl, sp + imm8");

    const U16 sp = Register(Register16::SP);
    const S8 imm8 = static_cast<S8>(ReadImm8());

    const U16 result = static_cast<U16>(sp + imm8);
    Register(Register16::HL, result);

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, ((sp & 0x0F) + (imm8 & 0x0F)) > 0x0F);
    Flag(Flags::C, ((sp & 0xFF) + (imm8 & 0xFF)) > 0xFF);
}

void CPU::LoadHLToSP()
{
    PrintInstruction("ld sp, hl");

    m_SP = Register(Register16::HL);
}

#pragma endregion

#pragma region 8-bit Load Instructions

void CPU::LoadImm8ToR8(Register8 reg)
{
    const U8 data = ReadImm8();

    PrintInstruction("ld r8, {:02X}", data);

    Register(reg, data);
}

void CPU::LoadR8ToR8(Register8 left, Register8 right)
{
    PrintInstruction("ld {}, {}", RegisterLiteral(left), RegisterLiteral(right));

    if (const auto bus = m_Bus.lock())
    {
        Register(left, Register(right));
    }
}

void CPU::LoadHighToAccumulator(Register8 reg)
{
    PrintInstruction("ldh a, [{}]", reg == Register8::C ? "c" : "imm8");

    if (const auto bus = m_Bus.lock())
    {
        const Address address = 0xFF00 + Register(reg);
        Register(Register8::A, bus->Read(address));
    }
}

void CPU::LoadHighFromAccumulator(Register8 reg)
{
    PrintInstruction("ldh [{}], a", reg == Register8::C ? "c" : "imm8");

    if (const auto bus = m_Bus.lock())
    {
        const Address address = 0xFF00 + Register(reg);
        if (address == 0xFF07)
        {
            std::println("Test");
            lastCounter = counter;
        }
        bus->Write(address, Register(Register8::A));
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

    Flag(Flags::Z, static_cast<U8>(result) == 0x00);
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

    Flag(Flags::Z, static_cast<U8>(result) == 0x00);
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

    Flag(Flags::Z, static_cast<U8>(result) == 0x00);
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

    Flag(Flags::Z, static_cast<U8>(result) == 0x00);
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
    PrintInstruction("or a, {}", RegisterLiteral(reg));

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
    const U8 value = Register(reg);
    Register(reg, value + 1);

    PrintInstruction("inc {}({:02X})", RegisterLiteral(reg), value);

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
    PrintInstruction("daa");

    U8 accumulator = Register(Register8::A);
    bool carry = Flag(Flags::C);
    bool sub = Flag(Flags::N);

    const U8 lsn = accumulator & 0x0F;
    U8 correction = 0;
    if (Flag(Flags::H) || (lsn > 9 && !sub)) correction = 0x06;

    carry = carry || (((accumulator & 0xF0) > 0x90 && correction == 0x06) && !sub);
    accumulator += Flag(Flags::N) ? -correction : correction;
    correction = 0;

    const U8 msn = accumulator >> 4;
    if (carry || (msn > 0x9 && !sub)) correction = 0x60;
    carry = carry || correction == 0x60;

    accumulator += Flag(Flags::N) ? -correction : correction;
    Register(Register8::A, accumulator);

    Flag(Flags::Z, accumulator == 0x00);
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

    const bool carry = Register(Register8::A) & 0x80;

    Register(Register8::A, static_cast<U8>((Register(Register8::A) << 1) | static_cast<U8>(Flag(Flags::C))));

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);
}

void CPU::RotateRightAccumulator()
{
    PrintInstruction("rra");

    const bool carry = Register(Register8::A) & 0x01;

    Register(Register8::A, static_cast<U8>((Register(Register8::A) >> 1) | (Flag(Flags::C) ? 0x80 : 0x00)));

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);
}

void CPU::RotateLeftCarry(Register8 reg)
{
    PrintInstruction("rlc r8");

    const bool carry = Register(reg) & 0x80;
    const U8 result = static_cast<U8>(Register(reg) << 1) | static_cast<U8>(carry);
    Register(reg, result);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);
}

void CPU::RotateRightCarry(Register8 reg)
{
    PrintInstruction("rrc r8");

    const bool carry = Register(reg) & 0x01;
    const U8 result = static_cast<U8>(Register(reg) >> 1) | (carry ? 0x80 : 0x00);
    Register(reg, result);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);
}

void CPU::RotateLeft(Register8 reg)
{
    PrintInstruction("rl r8");

    const bool carry = Register(reg) & 0x80;
    const U8 result = static_cast<U8>(Register(reg) << 1) | Flag(Flags::C);
    Register(reg, result);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);
}

void CPU::RotateRight(Register8 reg)
{
    PrintInstruction("rr r8");

    const bool carry = Register(reg) & 0x01;
    const U8 result = static_cast<U8>(Register(reg) >> 1) | (Flag(Flags::C) ? 0x80 : 0x00);
    Register(reg, result);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);
}

void CPU::ShiftLeft(Register8 reg)
{
    PrintInstruction("sla r8");

    const bool carry = Register(reg) & 0x80;
    Register(reg, static_cast<U8>(Register(reg) << 1));

    Flag(Flags::Z, Register(reg) == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);
}

void CPU::ShiftRight(Register8 reg)
{
    PrintInstruction("sra r8");

    const bool bit7 = Register(reg) & 0x80;
    const bool carry = Register(reg) & 0x01;
    Register(reg, static_cast<U8>(Register(reg) >> 1) | (bit7 ? 0x80 : 0x00));

    Flag(Flags::Z, Register(reg) == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);
}

void CPU::ShiftRightLogically(Register8 reg)
{
    PrintInstruction("srl r8");

    const bool carry = Register(reg) & 0x01;
    Register(reg, static_cast<U8>(Register(reg) >> 1));

    Flag(Flags::Z, Register(reg) == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);
}


void CPU::Swap(Register8 reg)
{
    PrintInstruction("swap r8");

    const U8 lsn = Register(reg) & 0x0F;
    const U8 hsn = Register(reg) & 0xF0;

    Register(reg, static_cast<U8>(lsn << 4) | static_cast<U8>(hsn >> 4));

    Flag(Flags::Z, Register(reg) == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, false);
}

#pragma endregion

#pragma region Control Flow Instructions

void CPU::Call()
{
    PrintInstruction("call imm16");

    const Address address = ReadImm16();
    Push(m_PC);
    m_PC = address;
}

void CPU::CallConditional(U8 cond)
{
    PrintInstruction("call cond, imm16");

    const Address address = ReadImm16();
    if (Condition(cond))
    {
        Push(m_PC);
        m_PC = address;
    }
}

void CPU::Jump(Register16 reg)
{
    const Address address = Register(reg);
    PrintInstruction("jp 0x{:04X}", address);
    m_PC = address;
}

void CPU::JumpConditional(U8 cond)
{
    PrintInstruction("jp cond, imm16");

    const Address address = ReadImm16();
    if (Condition(cond))
    {
        m_PC = address;
    }
}

void CPU::JumpRelative()
{
    PrintInstruction("jr imm8");

    const S8 offset = static_cast<S8>(ReadImm8());
    m_PC += offset;
}

void CPU::JumpRelativeConditional(U8 cond)
{
    const S8 offset = static_cast<S8>(ReadImm8());
    PrintInstruction("jr {}, {}({:02X})", ConditionLiteral(cond), offset, static_cast<U8>(offset));
    if (Condition(cond)) m_PC += offset;
}

void CPU::Return()
{
    PrintInstruction("ret");

    m_Interrupting = false;

    const Address address = Pop16();
    m_PC = address;
}

void CPU::ReturnI()
{
    PrintInstruction("reti");

    const Address address = Pop16();
    m_PC = address;

    m_IME = true;
}

void CPU::ReturnConditional(U8 cond)
{
    PrintInstruction("ret cond");

    if (Condition(cond))
    {
        const Address address = Pop16();
        m_PC = address;
    }
}


void CPU::Restart(U8 vec)
{
    PrintInstruction("rst vec");

    Push(m_PC);
    m_PC = static_cast<U16>(vec << 3);
}

#pragma endregion

#pragma region Miscellanious Instructions

void CPU::Stop() const
{
    PrintInstruction("Stop");
    system("pause");
}

void CPU::DisableInterrupts()
{
    PrintInstruction("di");

    m_IME = false;
}

void CPU::EnableInterrupts()
{
    PrintInstruction("ei");

    m_IME_Next_Cycle = true;
    m_Wait = 2;
}

void CPU::AddSP()
{
    PrintInstruction("add sp, imm8");

    const S8 imm8 = static_cast<S8>(ReadImm8());
    const U16 sp = m_SP;
    const U16 result = static_cast<U16>(sp + imm8);
    m_SP = result;

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, ((sp & 0x0F) + (imm8 & 0x0F)) > 0x0F);
    Flag(Flags::C, ((sp & 0xFF) + (imm8 & 0xFF)) > 0xFF);
}

#pragma endregion

#pragma region Bit Operations

void CPU::Bit(U8 bitIndex, Register8 reg)
{
    Flag(Flags::Z, (Register(reg) & static_cast<U8>(pow(2, bitIndex))) == 0);
    Flag(Flags::N, false);
    Flag(Flags::H, true);
}

void CPU::Reset(U8 bitIndex, Register8 reg)
{
    Register(reg, Register(reg) & ~static_cast<U8>(pow(2, bitIndex)));
}

void CPU::Set(U8 bitIndex, Register8 reg)
{
    Register(reg, Register(reg) | static_cast<U8>(pow(2, bitIndex)));
}

#pragma endregion

#pragma endregion
