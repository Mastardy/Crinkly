#include "CPU.hpp"

#include <filesystem>
#include <iostream>
#include <print>
#include <thread>

// #define PRINT_INSTRUCTION

CPU::CPU(const std::shared_ptr<Bus>& bus)
{
    m_Registers[std::to_underlying(Register8::A)] = 0;
    m_Registers[std::to_underlying(Register8::B)] = 0;
    m_Registers[std::to_underlying(Register8::C)] = 0;
    m_Registers[std::to_underlying(Register8::D)] = 0;
    m_Registers[std::to_underlying(Register8::E)] = 0;
    m_Registers[std::to_underlying(Register8::F)] = 0;
    m_Registers[std::to_underlying(Register8::H)] = 0;
    m_Registers[std::to_underlying(Register8::L)] = 0;

    m_Bus = bus;

    m_SP = 0x0000;
    m_PC = 0x0000;
    m_IME = false;
    m_IME_Next_Cycle = false;
    m_IME_Cancel = false;
    m_Interrupting = false;
    m_Halted = false;
    m_Wait = 0;

    m_Log.open(std::format("{}.log", bus->CartridgeName()));
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
    if (auto bus = m_Bus.lock())
    {
        bus->Write(0xFF00, 0xFF);
        bus->Write(0xFF05, 0x00);
        bus->Write(0xFF06, 0x00);
        bus->Write(0xFF07, 0x00);
        bus->Write(0xFF10, 0x80);
        bus->Write(0xFF11, 0xBF);
        bus->Write(0xFF12, 0xF3);
        bus->Write(0xFF14, 0xBF);
        bus->Write(0xFF16, 0x3F);
        bus->Write(0xFF17, 0x00);
        bus->Write(0xFF19, 0xBF);
        bus->Write(0xFF1A, 0x7F);
        bus->Write(0xFF1B, 0xFF);
        bus->Write(0xFF1C, 0x9F);
        bus->Write(0xFF1E, 0xBF);
        bus->Write(0xFF20, 0xFF);
        bus->Write(0xFF21, 0x00);
        bus->Write(0xFF22, 0x00);
        bus->Write(0xFF23, 0xBF);
        bus->Write(0xFF24, 0x77);
        bus->Write(0xFF25, 0xF3);
        bus->Write(0xFF26, 0xF1);
        bus->Write(0xFF40, 0x91);
        bus->Write(0xFF42, 0x00);
        bus->Write(0xFF43, 0x00);
        bus->Write(0xFF45, 0x00);
        bus->Write(0xFF47, 0xFC);
        bus->Write(0xFF48, 0xFF);
        bus->Write(0xFF49, 0xFF);
        bus->Write(0xFF4A, 0x00);
        bus->Write(0xFF4B, 0x00);
        bus->Write(0xFFFF, 0x00);
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
            return MemRead(Register(Register16::HL));
        }
        return 0;
    }

    return reg == Register8::Imm8 ? ReadImm8() : m_Registers[std::to_underlying(reg)];
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
        {
            auto a = m_Registers[std::to_underlying(Register8::A)];
            auto f = m_Registers[std::to_underlying(Register8::F)];
            return static_cast<U16>(a << 8) | f;
        }
    case Register16::BC:
        {
            auto b = m_Registers[std::to_underlying(Register8::B)];
            auto c = m_Registers[std::to_underlying(Register8::C)];
            return static_cast<U16>(b << 8) | c;
        }
    case Register16::DE:
        {
            auto d = m_Registers[std::to_underlying(Register8::D)];
            auto e = m_Registers[std::to_underlying(Register8::E)];
            return static_cast<U16>(d << 8) | e;
        }
    case Register16::HLi:
    case Register16::HLd:
    case Register16::HL:
        {
            auto h = m_Registers[std::to_underlying(Register8::H)];
            auto l = m_Registers[std::to_underlying(Register8::L)];
            return static_cast<U16>(h << 8) | l;
        }
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
            MemWrite(Register(Register16::HL), value);
        }
        return;
    }

    m_Registers[std::to_underlying(reg)] = value;
}

void CPU::Register(Register16 reg, const U16& value)
{
    switch (reg)
    {
    case Register16::AF:
        m_Registers[std::to_underlying(Register8::A)] = value >> 8;
        m_Registers[std::to_underlying(Register8::F)] = value & 0xF0;
        break;
    case Register16::BC:
        m_Registers[std::to_underlying(Register8::B)] = value >> 8;
        m_Registers[std::to_underlying(Register8::C)] = value & 0xFF;
        break;
    case Register16::DE:
        m_Registers[std::to_underlying(Register8::D)] = value >> 8;
        m_Registers[std::to_underlying(Register8::E)] = value & 0xFF;
        break;
    case Register16::HL:
        m_Registers[std::to_underlying(Register8::H)] = value >> 8;
        m_Registers[std::to_underlying(Register8::L)] = value & 0xFF;
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
            MemWrite(address, static_cast<U8>(value));
        }
        break;
    }
}

void CPU::Flag(Flags flag, bool set)
{
    m_Registers[std::to_underlying(Register8::F)] = set
                                                        ? m_Registers[std::to_underlying(Register8::F)] | static_cast<
                                                            U8>(flag)
                                                        : m_Registers[std::to_underlying(Register8::F)] & ~static_cast<
                                                            U8>(flag);
}

bool CPU::Flag(Flags flag)
{
    return m_Registers[std::to_underlying(Register8::F)] & static_cast<U8>(flag);
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
#ifdef PRINT_INSTRUCTION
        std::println("Reading from 0x{:04X} -> {:02X}", m_PC, bus->Read(m_PC));
#endif
        return MemRead(m_PC++);
    }

    return 0x00;
}

U16 CPU::ReadImm16()
{
    if (const auto bus = m_Bus.lock())
    {
#ifdef PRINT_INSTRUCTION
        std::println("Reading from 0x{:04X} -> {:02X}", m_PC, bus->Read(m_PC));
        std::println("Reading from 0x{:04X} -> {:02X}", m_PC + 1, bus->Read(m_PC + 1));
#endif
        auto low = MemRead(m_PC++);
        auto high = MemRead(m_PC++);
        return static_cast<U16>(low) | static_cast<U16>(high << 8);
    }

    return 0x0000;
}
#pragma endregion

#pragma region CPU Stack
void CPU::Push(U8 value)
{
    if (auto bus = m_Bus.lock())
    {
        MemWrite(--m_SP, value);
    }
}

void CPU::Push(U16 value)
{
    Push(static_cast<U8>(value >> 8));
    Push(static_cast<U8>(value & 0xFF));
}

U8 CPU::Push(Register16 reg)
{
    const U16 value = Register(reg);
    Push(value);
    return 16;
}


U8 CPU::Pop()
{
    if (auto bus = m_Bus.lock())
    {
        return MemRead(m_SP++);
    }
    exit(1);
}

U8 CPU::Pop(Register16 reg)
{
    U16 value = Pop();
    value |= static_cast<U16>(Pop() << 8);

    Register(reg, value);

    return 12;
}


U16 CPU::Pop16()
{
    U16 value = Pop();
    value |= static_cast<U16>(Pop() << 8);
    return value;
}
#pragma endregion
#pragma endregion

void CPU::SavePixels()
{
    /*
    if (const auto bus = m_Bus.lock())
    {
        std::string folder = std::format("frames/{}/Tile Blocks", bus->CartridgeName());
        std::filesystem::create_directories(folder);

        std::ofstream tileBlock(std::format("{}/{}.bmp", folder, frame));

        U8 header[54] = {
            0x42, 0x4D,
            0, 0, 0, 0,
            0, 0, 0, 0,
            54, 0, 0, 0,
            40, 0, 0, 0,
            128, 0, 0, 0,
            192, 0, 0, 0,
            1, 0,
            24, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0
        };

        int fileSize = 54 + 192 * 128 * 3;
        header[2] = static_cast<U8>(fileSize & 0xFF);
        header[3] = static_cast<U8>((fileSize >> 8) & 0xFF);
        header[4] = static_cast<U8>((fileSize >> 16) & 0xFF);
        header[5] = static_cast<U8>((fileSize >> 24) & 0xFF);

        tileBlock.write(reinterpret_cast<char*>(header), 54);

        for (S32 ty = 23; ty >= 0; ty--)
        {
            for (S32 y = 7; y >= 0; y--)
            {
                for (S32 tx = 0; tx < 16; tx++)
                {
                    for (S32 x = 0; x < 8; x++)
                    {
                        if (tx == 0 && x == 0 || (tx == 15 && x == 7))
                        {
                            if (ty < 8)
                            {
                                tileBlock.put(static_cast<U8>(255));
                                tileBlock.put(0);
                                tileBlock.put(0);
                            }
                            else if (ty < 16)
                            {
                                tileBlock.put(0);
                                tileBlock.put(static_cast<U8>(255));
                                tileBlock.put(0);
                            }
                            else
                            {
                                tileBlock.put(0);
                                tileBlock.put(0);
                                tileBlock.put(static_cast<U8>(255));
                            }
                            continue;
                        }
                        else if (y == 7)
                        {
                            if (ty == 7)
                            {
                                tileBlock.put(static_cast<U8>(255));
                                tileBlock.put(0);
                                tileBlock.put(0);
                                continue;
                            }
                            else if (ty == 15)
                            {
                                tileBlock.put(0);
                                tileBlock.put(static_cast<U8>(255));
                                tileBlock.put(0);
                                continue;
                            }
                            else if (ty == 23)
                            {
                                tileBlock.put(0);
                                tileBlock.put(0);
                                tileBlock.put(static_cast<U8>(255));
                                continue;
                            }
                        }
                        else if (y == 0)
                        {
                            if (ty == 0)
                            {
                                tileBlock.put(static_cast<U8>(255));
                                tileBlock.put(0);
                                tileBlock.put(0);
                                continue;
                            }
                            else if (ty == 8)
                            {
                                tileBlock.put(0);
                                tileBlock.put(static_cast<U8>(255));
                                tileBlock.put(0);
                                continue;
                            }
                            else if (ty == 16)
                            {
                                tileBlock.put(0);
                                tileBlock.put(0);
                                tileBlock.put(static_cast<U8>(255));
                                continue;
                            }
                        }

                        U16 byteStride = static_cast<U16>(ty * 16 + tx) * 16;
                        U16 address = 0x8000 + byteStride + static_cast<U16>(y * 2);

                        U8 leftByte = bus->Read(address);
                        U8 rightByte = bus->Read(address + 1);

                        U8 lsb = (leftByte >> static_cast<U8>(7 - x)) & 0x01;
                        U8 msb = (rightByte >> static_cast<U8>(7 - x)) & 0x01;

                        U8 pixel = static_cast<U8>(msb << 1) | lsb;

                        tileBlock.put(pixel * 85);
                        tileBlock.put(pixel * 85);
                        tileBlock.put(pixel * 85);
                    }
                }
            }
        }

        folder = std::format("frames/{}/Tile Maps", bus->CartridgeName());
        std::filesystem::create_directories(folder);

        std::ofstream tileMap1(std::format("{}/1-{}.bmp", folder, frame));
        std::ofstream tileMap2(std::format("{}/2-{}.bmp", folder, frame));

        U8 tileMapHeader[54] = {
            0x42, 0x4D,
            0, 0, 0, 0,
            0, 0, 0, 0,
            54, 0, 0, 0,
            40, 0, 0, 0,
            0, 1, 0, 0,
            0, 1, 0, 0,
            1, 0,
            24, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0
        };

        fileSize = 54 + 256 * 256 * 3;
        tileMapHeader[2] = static_cast<U8>(fileSize & 0xFF);
        tileMapHeader[3] = static_cast<U8>((fileSize >> 8) & 0xFF);
        tileMapHeader[4] = static_cast<U8>((fileSize >> 16) & 0xFF);
        tileMapHeader[5] = static_cast<U8>((fileSize >> 24) & 0xFF);

        tileMap1.write(reinterpret_cast<char*>(tileMapHeader), 54);
        tileMap2.write(reinterpret_cast<char*>(tileMapHeader), 54);

        for (S32 ty = 31; ty >= 0; ty--)
        {
            for (S32 y = 7; y >= 0; y--)
            {
                for (S32 tx = 0; tx < 32; tx++)
                {
                    for (S32 x = 0; x < 8; x++)
                    {
                        U16 addressOffset = 0x9800 + static_cast<U16>(ty * 32 + tx);
                        U16 tileIndex = bus->Read(addressOffset);
                        U16 address = 0x8000 + tileIndex * 16 + static_cast<U16>(y * 2);

                        U8 leftByte = bus->Read(address);
                        U8 rightByte = bus->Read(address + 1);

                        U8 lsb = (leftByte >> static_cast<U8>(7 - x)) & 0x01;
                        U8 msb = (rightByte >> static_cast<U8>(7 - x)) & 0x01;

                        U8 pixel = static_cast<U8>(msb << 1) | lsb;

                        tileMap1.put(pixel * 85);
                        tileMap1.put(pixel * 85);
                        tileMap1.put(pixel * 85);

                        addressOffset = 0x9C00 + static_cast<U16>(ty * 32 + tx);
                        tileIndex = bus->Read(addressOffset);
                        address = 0x8000 + tileIndex * 16 + static_cast<U16>(y * 2);

                        leftByte = bus->Read(address);
                        rightByte = bus->Read(address + 1);

                        lsb = (leftByte >> static_cast<U8>(7 - x)) & 0x01;
                        msb = (rightByte >> static_cast<U8>(7 - x)) & 0x01;

                        pixel = static_cast<U8>(msb << 1) | lsb;

                        tileMap2.put(pixel * 85);
                        tileMap2.put(pixel * 85);
                        tileMap2.put(pixel * 85);
                    }
                }
            }
        }

        std::cin.get();
    }
    */
}

U8 CPU::MemRead(const Address address)
{
    if (m_Tick) m_Tick(4);
    m_TickedCycles += 4;
    if (const auto bus = m_Bus.lock()) return bus->Read(address);
    return 0xFF;
}

void CPU::MemWrite(const Address address, const U8 value)
{
    if (m_Tick) m_Tick(4);
    m_TickedCycles += 4;
    if (const auto bus = m_Bus.lock()) bus->Write(address, value);
}

U8 CPU::Step()
{
    m_TickedCycles = 0;
    const U8 total = StepInstruction();
    return total > m_TickedCycles ? total - m_TickedCycles : 0;
}

U8 CPU::StepInstruction()
{
    const auto enableIme = m_IME_Next_Cycle;
    m_IME_Next_Cycle = false;
    m_IME_Cancel = false;

    const U8 cycles = ExecuteInstruction();

    if (enableIme && !m_IME_Cancel) m_IME = true;
    return cycles;
}

U8 CPU::ExecuteInstruction()
{
    if (auto bus = m_Bus.lock())
    {
        if (m_IME)
        {
            auto interrupt = bus->Read(0xFF0F) & bus->Read(0xFFFF);
            if (interrupt != 0x00)
            {
                m_IME = false;
                m_IME_Next_Cycle = false;
                m_IME_Cancel = true;

                m_Interrupting = true;

                Push(m_PC);
                if (interrupt & 0x01) // VBlank
                {
                    bus->Write(0xFF0F, bus->Read(0xFF0F) & ~0x01);
                    m_PC = 0x0040;
                }
                else if (interrupt & 0x02) // STAT 
                {
                    bus->Write(0xFF0F, bus->Read(0xFF0F) & ~0x02);
                    m_PC = 0x0048;
                }
                else if (interrupt & 0x04) // Timer
                {
                    bus->Write(0xFF0F, bus->Read(0xFF0F) & ~0x04);
                    m_PC = 0x0050;
                }
                else if (interrupt & 0x08) // Serial
                {
                    bus->Write(0xFF0F, bus->Read(0xFF0F) & ~0x08);
                    m_PC = 0x0058;
                }
                else if (interrupt & 0x10) // Joypad
                {
                    bus->Write(0xFF0F, bus->Read(0xFF0F) & ~0x10);
                    m_PC = 0x0060;
                }
                m_Halted = false;
                return 20;
            }
        }

        if (m_Halted)
        {
            const auto pending = bus->Read(0xFF0F) & bus->Read(0xFFFF);
            if (pending == 0)
                return 4;
            m_Halted = false; // IME=false HALT bug: exit halt, execute instruction after HALT
        }

        /*
        U8 PCMEM[4];
        PCMEM[0] = bus->Read(m_PC);
        PCMEM[1] = bus->Read(m_PC + 1);
        PCMEM[2] = bus->Read(m_PC + 2);
        PCMEM[3] = bus->Read(m_PC + 3);
        
        auto str = std::format(
            "A:{:02X} F:{:02X} B:{:02X} C:{:02X} D:{:02X} E:{:02X} H:{:02X} L:{:02X} SP:{:04X} PC:{:04X} PCMEM:{:02X},{:02X},{:02X},{:02X}",
            Register(Register8::A), Register(Register8::F), Register(Register8::B), Register(Register8::C),
            Register(Register8::D), Register(Register8::E), Register(Register8::H), Register(Register8::L),
            Register(Register16::SP), Register(Register16::PC), PCMEM[0], PCMEM[1], PCMEM[2], PCMEM[3]);
        m_Log << str << '\n';*/

        const auto opcode = MemRead(m_PC++);
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
            std::print("{}", static_cast<char>(bus->Read(0xFF01)));
            bus->Write(0xFF02, 0x0);
        }

#ifdef DEBUG_BLARGG
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
#endif

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
                    return 4;
                case 0x07:
                    return RotateLeftCarryAccumulator(); // rlca
                case 0x08:
                    return LoadFromStackPointer(); // ld [imm16], sp
                case 0x0F:
                    return RotateRightCarryAccumulator(); // rrca
                case 0x10:
                    return Stop(); // stop
                case 0x17:
                    return RotateLeftAccumulator(); // rla
                case 0x18:
                    return JumpRelative(); // jr imm8
                case 0x1F:
                    return RotateRightAccumulator(); // rra
                case 0x27:
                    return DecimalAdjustAccumulator(); // daa
                case 0x2F:
                    return ComplementAccumulator(); // cpl
                case 0x37:
                    return SetCarryFlag(); // scf
                case 0x3F:
                    return ComplementCarryFlag(); // ccf
                default:
                    U8 reg = (params & 0x30) >> 4;
                    U8 rightParam = params & 0x0F;

                    if (rightParam == 0x1)
                    {
                        return LoadImm16ToR16(R16[reg]); // ld r16, imm16
                    }
                    else if (rightParam == 0x2)
                    {
                        return LoadAccumulatorToR16Address(R16mem[reg]); // ld [r16mem], a
                    }
                    else if (rightParam == 0x3)
                    {
                        return Increment(R16[reg]); // inc r16
                    }
                    else if (rightParam == 0x9)
                    {
                        return Add(R16[reg]); // add hl, r16
                    }
                    else if (rightParam == 0xA)
                    {
                        return LoadR16AddressToAccumulator(R16mem[reg]); // ld a, [r16mem]
                    }
                    else if (rightParam == 0xB)
                    {
                        return Decrement(R16[reg]); // dec r16
                    }

                    reg = (params & 0x38) >> 3;
                    rightParam = params & 0x07;

                    if (rightParam == 0x4)
                    {
                        return Increment(R8[reg]); // inc r8
                    }
                    else if (rightParam == 0x5)
                    {
                        return Decrement(R8[reg]); // dec r8
                    }
                    else if (rightParam == 0x6)
                    {
                        return LoadImm8ToR8(R8[reg]); // ld r8, imm8
                    }

                    if ((params & 0x20) == 0x20 && rightParam == 0)
                    {
                        return JumpRelativeConditional((params >> 3) & 0x03); // jr cc, imm8
                    }
                }
                return 0;
            }
        case 0x40: // Block 1: 8-bit Register-To-Register loads
            {
                if (params == 0x36)
                {
                    m_Halted = true;
                    return 4;
                }
                else
                {
                    const U8 dest = (params >> 3) & 0x07;
                    const U8 src = params & 0x07;
                    return LoadR8ToR8(R8[dest], R8[src]); // ld r8, r8
                }
            }
        case 0x80:
            {
                const Register8 operand = R8[params & 0x07];
                switch ((params & 0x38) >> 3)
                {
                case 0x0:
                    return Add(operand); // add a, r8
                case 0x1:
                    return Adc(operand); // adc a, r8
                case 0x2:
                    return Sub(operand); // sub a, r8
                case 0x3:
                    return Sbc(operand); // sbc a, r8
                case 0x4:
                    return And(operand); // and a, r8
                case 0x5:
                    return Xor(operand); // xor a, r8
                case 0x6:
                    return Or(operand); // or a, r8
                case 0x7:
                    return Cp(operand); // cp a, r8
                }
                return 0;
            }
        case 0xC0:
            {
                switch (params)
                {
                case 0x03:
                    return Jump(Register16::Imm16); // jp imm16
                case 0x06:
                    return Add(Register8::Imm8); // add a, imm8
                case 0x09:
                    return Return(); // ret
                case 0x0B:
                    {
                        const U8 suffix = MemRead(m_PC++);
                        const U8 suffixBlock = (suffix & 0xC0) >> 6;
                        const U8 operand = suffix & 0x7;
                        const U8 bitIndex = (suffix & 0x38) >> 3;

                        switch (suffixBlock)
                        {
                        case 0x0:
                            switch (bitIndex)
                            {
                            case 0x0:
                                return RotateLeftCarry(R8[operand]); // rlc r8
                            case 0x1:
                                return RotateRightCarry(R8[operand]); // rrc r8
                            case 0x2:
                                return RotateLeft(R8[operand]); // rl r8
                            case 0x3:
                                return RotateRight(R8[operand]); // rr r8
                            case 0x4:
                                return ShiftLeft(R8[operand]); // sla r8
                            case 0x5:
                                return ShiftRight(R8[operand]); // sra r8
                            case 0x6:
                                return Swap(R8[operand]); // swap r8
                            case 0x7:
                                return ShiftRightLogically(R8[operand]); // srl r8
                            }
                            return 0;
                        case 0x1:
                            return Bit(bitIndex, R8[operand]); // bit b, r8
                        case 0x2:
                            return Reset(bitIndex, R8[operand]); // reset b, r8
                        case 0x3:
                            return Set(bitIndex, R8[operand]); // set b, r8
                        }
                        return 0;
                    }
                case 0x0D:
                    return Call(); // call imm16
                case 0x0E:
                    return Adc(Register8::Imm8); // adc a, imm8
                case 0x16:
                    return Sub(Register8::Imm8); // sub a, imm8
                case 0x19:
                    return ReturnI(); // reti
                case 0x1E:
                    return Sbc(Register8::Imm8); // sbc a, imm8
                case 0x20:
                    return LoadHighFromAccumulator(Register8::Imm8); // ldh [imm8], a
                case 0x22:
                    return LoadHighFromAccumulator(Register8::C); // ldh [c], a
                case 0x26:
                    return And(Register8::Imm8); // and a, imm8
                case 0x28:
                    return AddSP(); // add sp, imm8
                case 0x29:
                    return Jump(Register16::HL); // jp hl
                case 0x2A:
                    return LoadAccumulatorToR16Address(Register16::Imm16); // ld [imm16], a
                case 0x2E:
                    return Xor(Register8::Imm8); // xor a, imm8
                case 0x30:
                    return LoadHighToAccumulator(Register8::Imm8); // ldh a, [imm8]
                case 0x32:
                    return LoadHighToAccumulator(Register8::C); // ldh a, [c]
                case 0x33:
                    return DisableInterrupts(); // di
                case 0x36:
                    return Or(Register8::Imm8); // or a, imm8
                case 0x38:
                    return LoadSPOffsetToHL(); // ld hl, [sp + imm8]
                case 0x39:
                    return LoadHLToSP(); // ld sp, hl
                case 0x3A:
                    return LoadR16AddressToAccumulator(Register16::Imm16); // ld a, [imm16]
                case 0x3B:
                    return EnableInterrupts(); // ei
                case 0x3E:
                    return Cp(Register8::Imm8); // cp a, imm8
                default:
                    const U8 right = params & 0x07;
                    const U8 cond = (params & 0x18) >> 3;
                    const U8 tgt3 = (params & 0x38) >> 3;
                    const Register16 rstk = R16stk[(params & 0x30) >> 4];

                    switch (right)
                    {
                    case 0x0:
                        return ReturnConditional(cond); // ret cc
                    case 0x1:
                        return Pop(rstk); // pop r16
                    case 0x2:
                        return JumpConditional(cond); // jp cc, imm16
                    case 0x4:
                        return CallConditional(cond); // call cc, imm16
                    case 0x5:
                        return Push(rstk); // push r16
                    case 0x7:
                        return Restart(tgt3); // rst vec
                    }
                }
            }
        }

        return 0;
    }
    else
    {
        std::cerr << "Bus is not available!";
        exit(1);
    }
}

void CPU::SetPC(U16 address)
{
    m_PC = address;
}

template <class... Types>
void CPU::PrintInstruction(const std::format_string<Types...>& text, [[maybe_unused]] Types&&... args)
{
#ifdef PRINT_INSTRUCTION
    std::println(text, std::forward<Types>(args)...);
#endif
}

#pragma region Instructions

#pragma region 16-bit Load Instructions

U8 CPU::LoadFromStackPointer()
{
    U16 address = ReadImm16();

    PrintInstruction("ld {:04X}, sp[{:04X}]", address, Register(Register16::SP));

    if (const auto bus = m_Bus.lock())
    {
        MemWrite(address++, static_cast<U8>(Register(Register16::SP) & 0xFF));
        MemWrite(address, static_cast<U8>(Register(Register16::SP) >> 8));
    }

    return 20;
}

U8 CPU::LoadImm16ToR16(Register16 reg)
{
    const U16 data = ReadImm16();
    PrintInstruction("ld {}, imm16", RegisterLiteral(reg), data);
    Register(reg, data);

    return 12;
}

U8 CPU::LoadAccumulatorToR16Address(Register16 reg)
{
    PrintInstruction("ld [{}], a", RegisterLiteral(reg));

    if (const auto bus = m_Bus.lock())
    {
        const U8 data = Register(Register8::A);
        const Address address = Register(reg);
        MemWrite(address, data);

        if (reg == Register16::HLd)
        {
            if (address == 0x0000)
            {
                Register(Register16::HL, 0xFFFF);
            }
            else
            {
                Register(Register16::HL, address - 1);
            }
        }
        else if (reg == Register16::HLi)
        {
            if (address == 0xFFFF)
            {
                Register(Register16::HL, 0x0000);
            }
            else
            {
                Register(Register16::HL, address + 1);
            }
        }
    }

    return reg == Register16::Imm16 ? 16 : 8;
}

U8 CPU::LoadR16AddressToAccumulator(Register16 reg)
{
    PrintInstruction("ld a, {}", RegisterLiteral(reg));

    if (const auto bus = m_Bus.lock())
    {
        const Address address = Register(reg);
        const U8 data = MemRead(address);
        Register(Register8::A, data);

        if (address == 0xFF0F)
        {
            std::println("Address: {:04X} -> {:02X}", address, data);
        }

        if (reg == Register16::HLd)
        {
            if (Register(Register16::HL) == 0x0000)
            {
                Register(Register16::HL, 0xFFFF);
            }
            else
            {
                Register(Register16::HL, Register(Register16::HL) - 1);
            }
        }
        else if (reg == Register16::HLi)
        {
            if (Register(Register16::HL) == 0xFFFF)
            {
                Register(Register16::HL, 0x0000);
            }
            else
            {
                Register(Register16::HL, Register(Register16::HL) + 1);
            }
        }
    }

    return reg == Register16::Imm16 ? 16 : 8;
}

U8 CPU::LoadSPOffsetToHL()
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

    return 12;
}

U8 CPU::LoadHLToSP()
{
    PrintInstruction("ld sp, hl");

    m_SP = Register(Register16::HL);

    return 8;
}

#pragma endregion

#pragma region 8-bit Load Instructions

U8 CPU::LoadImm8ToR8(Register8 reg)
{
    const U8 data = ReadImm8();

    PrintInstruction("ld r8, {:02X}", data);

    Register(reg, data);

    return reg == Register8::HL ? 12 : 8;
}

U8 CPU::LoadR8ToR8(Register8 left, Register8 right)
{
    PrintInstruction("ld {}, {}", RegisterLiteral(left), RegisterLiteral(right));

    if (const auto bus = m_Bus.lock())
    {
        Register(left, Register(right));
    }

    return (left == Register8::HL || right == Register8::HL) ? 8 : 4;
}

U8 CPU::LoadHighToAccumulator(Register8 reg)
{
    PrintInstruction("ldh a, [{}]", RegisterLiteral(reg));

    if (const auto bus = m_Bus.lock())
    {
        const Address address = 0xFF00 + Register(reg);
        Register(Register8::A, MemRead(address));
    }

    return reg == Register8::Imm8 ? 12 : 8;
}

U8 CPU::LoadHighFromAccumulator(Register8 reg)
{
    PrintInstruction("ldh [{}], a", RegisterLiteral(reg));

    if (const auto bus = m_Bus.lock())
    {
        const Address address = 0xFF00 + Register(reg);
        MemWrite(address, Register(Register8::A));
    }

    return reg == Register8::Imm8 ? 12 : 8;
}

#pragma endregion

#pragma region 8-bit Arithmetic & Logical Instructions

U8 CPU::Add(Register8 reg)
{
    PrintInstruction("add a, {}", RegisterLiteral(reg));

    const U8 accumulator = Register(Register8::A);
    const U8 value = Register(reg);
    const U16 result = accumulator + value;

    Register(Register8::A, static_cast<U8>(result));

    Flag(Flags::Z, static_cast<U8>(result) == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, (accumulator & 0x0F) + (value & 0x0F) > 0x0F);
    Flag(Flags::C, result > 0xFF);

    return (reg == Register8::HL || reg == Register8::Imm8) ? 8 : 4;
}

U8 CPU::Adc(Register8 reg)
{
    PrintInstruction("adc a, {}", RegisterLiteral(reg));

    const bool carry = Flag(Flags::C);
    const U8 accumulator = Register(Register8::A);
    const U8 value = Register(reg);
    const U16 result = accumulator + value + carry;

    Register(Register8::A, static_cast<U8>(result));

    Flag(Flags::Z, static_cast<U8>(result) == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, ((accumulator & 0x0F) + (value & 0x0F) + carry) > 0x0F);
    Flag(Flags::C, result > 0xFF);

    return (reg == Register8::HL || reg == Register8::Imm8) ? 8 : 4;
}

U8 CPU::Sub(Register8 reg)
{
    PrintInstruction("sub a, {}", RegisterLiteral(reg));

    const U8 accumulator = Register(Register8::A);
    const U8 value = Register(reg);
    const U16 result = accumulator - value;

    Register(Register8::A, static_cast<U8>(result));

    Flag(Flags::Z, static_cast<U8>(result) == 0x00);
    Flag(Flags::N, true);
    Flag(Flags::H, (accumulator & 0x0F) < (value & 0x0F));
    Flag(Flags::C, accumulator < value);

    return (reg == Register8::HL || reg == Register8::Imm8) ? 8 : 4;
}

U8 CPU::Sbc(Register8 reg)
{
    PrintInstruction("sbc a, {}", RegisterLiteral(reg));

    const bool carry = Flag(Flags::C);
    const U8 accumulator = Register(Register8::A);
    const U8 value = Register(reg);
    const U16 result = accumulator - value - carry;

    Register(Register8::A, static_cast<U8>(result));

    Flag(Flags::Z, static_cast<U8>(result) == 0x00);
    Flag(Flags::N, true);
    Flag(Flags::H, (accumulator & 0x0F) < ((value & 0x0F) + carry));
    Flag(Flags::C, accumulator < (value + carry));

    return (reg == Register8::HL || reg == Register8::Imm8) ? 8 : 4;
}

U8 CPU::And(Register8 reg)
{
    PrintInstruction("and a, {}", RegisterLiteral(reg));

    const U8 accumulator = Register(Register8::A);
    const U8 value = Register(reg);
    const U8 result = accumulator & value;

    Register(Register8::A, static_cast<U8>(result));

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, true);
    Flag(Flags::C, false);

    return (reg == Register8::HL || reg == Register8::Imm8) ? 8 : 4;
}

U8 CPU::Xor(Register8 reg)
{
    PrintInstruction("xor a, {}", RegisterLiteral(reg));

    const U8 accumulator = Register(Register8::A);
    const U8 value = Register(reg);
    const U8 result = accumulator ^ value;

    Register(Register8::A, result);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, false);

    return (reg == Register8::HL || reg == Register8::Imm8) ? 8 : 4;
}

U8 CPU::Or(Register8 reg)
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

    return (reg == Register8::HL || reg == Register8::Imm8) ? 8 : 4;
}

U8 CPU::Cp(Register8 reg)
{
    PrintInstruction("cp a, r8");

    const U8 accumulator = Register(Register8::A);
    const U8 value = Register(reg);
    const U8 result = accumulator - value;

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, true);
    Flag(Flags::H, (accumulator & 0x0F) < (value & 0x0F));
    Flag(Flags::C, accumulator < value);

    return (reg == Register8::HL || reg == Register8::Imm8) ? 8 : 4;
}

U8 CPU::Increment(Register8 reg)
{
    const U8 value = Register(reg);
    const U8 result = value + 1;
    Register(reg, result);

    PrintInstruction("inc {}({:02X})", RegisterLiteral(reg), value);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, (value & 0xF) == 0xF);

    return reg == Register8::HL ? 12 : 4;
}

U8 CPU::Decrement(Register8 reg)
{
    PrintInstruction("dec r8");

    const U8 value = Register(reg);
    const U8 result = value - 1;
    Register(reg, result);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, true);
    Flag(Flags::H, (value & 0x0F) == 0x0);

    return reg == Register8::HL ? 12 : 4;
}

U8 CPU::DecimalAdjustAccumulator()
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

    return 4;
}

U8 CPU::ComplementAccumulator()
{
    PrintInstruction("cpl");

    Register(Register8::A, static_cast<U8>(~Register(Register8::A)));

    Flag(Flags::N, true);
    Flag(Flags::H, true);

    return 4;
}

U8 CPU::SetCarryFlag()
{
    PrintInstruction("scf");

    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, true);

    return 4;
}

U8 CPU::ComplementCarryFlag()
{
    PrintInstruction("ccf");

    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, !Flag(Flags::C));

    return 4;
}

#pragma endregion

#pragma region 16-bit Arithmetic

U8 CPU::Add(Register16 reg)
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

    return 8;
}

U8 CPU::Increment(Register16 reg)
{
    PrintInstruction("inc r16");

    Register(reg, Register(reg) + 1);

    return 8;
}

U8 CPU::Decrement(Register16 reg)
{
    PrintInstruction("dec r16");

    Register(reg, Register(reg) - 1);

    return 8;
}


#pragma endregion

#pragma region Rotate, Shift, Bit Instructions

U8 CPU::RotateLeftCarryAccumulator()
{
    PrintInstruction("rlca");

    const bool carry = Register(Register8::A) & 0x80;
    Register(Register8::A, static_cast<U8>((Register(Register8::A) << 1) | static_cast<U8>(carry)));

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);

    return 4;
}

U8 CPU::RotateRightCarryAccumulator()
{
    PrintInstruction("rrca");

    const bool carry = Register(Register8::A) & 0x01;
    Register(Register8::A, static_cast<U8>((Register(Register8::A) >> 1) | (carry ? 0x80 : 0x00)));

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);

    return 4;
}

U8 CPU::RotateLeftAccumulator()
{
    PrintInstruction("rla");

    const bool carry = Register(Register8::A) & 0x80;

    Register(Register8::A, static_cast<U8>((Register(Register8::A) << 1) | static_cast<U8>(Flag(Flags::C))));

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);

    return 4;
}

U8 CPU::RotateRightAccumulator()
{
    PrintInstruction("rra");

    const bool carry = Register(Register8::A) & 0x01;

    Register(Register8::A, static_cast<U8>((Register(Register8::A) >> 1) | (Flag(Flags::C) ? 0x80 : 0x00)));

    Flag(Flags::Z, false);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);

    return 4;
}

U8 CPU::RotateLeftCarry(Register8 reg)
{
    PrintInstruction("rlc r8");

    const U8 value = Register(reg);
    const bool carry = value & 0x80;
    const U8 result = static_cast<U8>(value << 1) | static_cast<U8>(carry);
    Register(reg, result);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);

    return reg == Register8::HL ? 16 : 8;
}

U8 CPU::RotateRightCarry(Register8 reg)
{
    PrintInstruction("rrc r8");

    const U8 value = Register(reg);
    const bool carry = value & 0x01;
    const U8 result = static_cast<U8>(value >> 1) | (carry ? 0x80 : 0x00);
    Register(reg, result);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);

    return reg == Register8::HL ? 16 : 8;
}

U8 CPU::RotateLeft(Register8 reg)
{
    PrintInstruction("rl r8");

    const U8 value = Register(reg);
    const bool carry = value & 0x80;
    const U8 result = static_cast<U8>(value << 1) | static_cast<U8>(Flag(Flags::C));
    Register(reg, result);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);

    return reg == Register8::HL ? 16 : 8;
}

U8 CPU::RotateRight(Register8 reg)
{
    PrintInstruction("rr r8");

    const U8 value = Register(reg);
    const bool carry = value & 0x01;
    const U8 result = static_cast<U8>(value >> 1) | (Flag(Flags::C) ? 0x80 : 0x00);
    Register(reg, result);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);

    return reg == Register8::HL ? 16 : 8;
}

U8 CPU::ShiftLeft(Register8 reg)
{
    PrintInstruction("sla r8");

    const U8 value = Register(reg);
    const bool carry = value & 0x80;
    const U8 result = static_cast<U8>(value << 1);
    Register(reg, result);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);

    return reg == Register8::HL ? 16 : 8;
}

U8 CPU::ShiftRight(Register8 reg)
{
    PrintInstruction("sra r8");

    const U8 value = Register(reg);
    const bool bit7 = value & 0x80;
    const bool carry = value & 0x01;
    const U8 result = static_cast<U8>(value >> 1) | (bit7 ? 0x80 : 0x00);
    Register(reg, result);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);

    return reg == Register8::HL ? 16 : 8;
}

U8 CPU::ShiftRightLogically(Register8 reg)
{
    PrintInstruction("srl r8");

    const U8 value = Register(reg);
    const bool carry = value & 0x01;
    const U8 result = static_cast<U8>(value >> 1);
    Register(reg, result);

    Flag(Flags::Z, result == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, carry);

    return reg == Register8::HL ? 16 : 8;
}


U8 CPU::Swap(Register8 reg)
{
    PrintInstruction("swap r8");

    const U8 value = Register(reg);
    const U8 lsn = value & 0x0F;
    const U8 hsn = value & 0xF0;

    Register(reg, static_cast<U8>(lsn << 4) | static_cast<U8>(hsn >> 4));

    Flag(Flags::Z, value == 0x00);
    Flag(Flags::N, false);
    Flag(Flags::H, false);
    Flag(Flags::C, false);

    return reg == Register8::HL ? 16 : 8;
}

#pragma endregion

#pragma region Control Flow Instructions

U8 CPU::Call()
{
    PrintInstruction("call imm16");

    const Address address = ReadImm16();
    Push(m_PC);
    m_PC = address;

    return 24;
}

U8 CPU::CallConditional(U8 cond)
{
    PrintInstruction("call cond, imm16");

    const Address address = ReadImm16();
    if (Condition(cond))
    {
        Push(m_PC);
        m_PC = address;
        return 24;
    }

    return 12;
}

U8 CPU::Jump(Register16 reg)
{
    const Address address = Register(reg);
    PrintInstruction("jp 0x{:04X}", address);
    m_PC = address;
    return reg == Register16::HL ? 4 : 16;
}

U8 CPU::JumpConditional(U8 cond)
{
    PrintInstruction("jp cond, imm16");

    const Address address = ReadImm16();
    if (Condition(cond))
    {
        m_PC = address;
        return 16;
    }

    return 12;
}

U8 CPU::JumpRelative()
{
    PrintInstruction("jr imm8");

    const S8 offset = static_cast<S8>(ReadImm8());
    m_PC += offset;

    return 12;
}

U8 CPU::JumpRelativeConditional(U8 cond)
{
    const S8 offset = static_cast<S8>(ReadImm8());
    PrintInstruction("jr {}, {}({:02X})", ConditionLiteral(cond), offset, static_cast<U8>(offset));

    if (!Condition(cond)) return 8;

    m_PC += offset;
    return 12;
}

U8 CPU::Return()
{
    PrintInstruction("ret");

    m_Interrupting = false;

    const Address address = Pop16();
    m_PC = address;

    return 16;
}

U8 CPU::ReturnI()
{
    PrintInstruction("reti");

    const Address address = Pop16();
    m_PC = address;

    m_IME = true;

    return 16;
}

U8 CPU::ReturnConditional(U8 cond)
{
    PrintInstruction("ret cond");

    if (Condition(cond))
    {
        const Address address = Pop16();
        m_PC = address;

        return 20;
    }

    return 8;
}


U8 CPU::Restart(U8 vec)
{
    PrintInstruction("rst vec");

    Push(m_PC);
    m_PC = static_cast<U16>(vec << 3);

    return 16;
}

#pragma endregion

#pragma region Miscellanious Instructions

U8 CPU::Stop() const
{
    PrintInstruction("Stop");

    return 4;
}

U8 CPU::DisableInterrupts()
{
    PrintInstruction("di");
    m_IME = false;
    m_IME_Cancel = true;
    return 4;
}

U8 CPU::EnableInterrupts()
{
    PrintInstruction("ei");

    m_IME_Next_Cycle = true;
    m_Wait = 2;

    return 4;
}

U8 CPU::AddSP()
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

    return 16;
}

#pragma endregion

#pragma region Bit Operations

U8 CPU::Bit(U8 bitIndex, Register8 reg)
{
    Flag(Flags::Z, (Register(reg) & static_cast<U8>(pow(2, bitIndex))) == 0);
    Flag(Flags::N, false);
    Flag(Flags::H, true);

    return reg == Register8::HL ? 12 : 8;
}

U8 CPU::Reset(U8 bitIndex, Register8 reg)
{
    Register(reg, Register(reg) & ~static_cast<U8>(pow(2, bitIndex)));

    return reg == Register8::HL ? 16 : 8;
}

U8 CPU::Set(U8 bitIndex, Register8 reg)
{
    Register(reg, Register(reg) | static_cast<U8>(pow(2, bitIndex)));

    return reg == Register8::HL ? 16 : 8;
}

#pragma endregion

#pragma endregion
