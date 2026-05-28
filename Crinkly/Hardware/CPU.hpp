#pragma once
#include <format>
#include <fstream>
#include <memory>

#include "Bus.hpp"
#include "LCD.hpp"
#include "Utility/Types.hpp"

class CPU
{
    std::ofstream m_Log;

public:
    enum class Register8 : U8
    {
        A,
        B,
        C,
        D,
        E,
        F,
        H,
        L,
        HL,
        Imm8
    };

    enum class Register16 : U8
    {
        AF,
        BC,
        DE,
        HL,
        SP,
        PC,
        HLi,
        HLd,
        Imm16
    };

    enum class Flags : U8
    {
        Z = 0x80,
        N = 0x40,
        H = 0x20,
        C = 0x10
    };

public:
    CPU(const std::shared_ptr<Bus>& bus, const std::shared_ptr<LCD>& lcd);
    ~CPU();

    void SavePixels();
    void Bootstrap();

    U8 Register(Register8 reg);
    U16 Register(Register16 reg);

    void Register(Register8 reg, U8 value);
    void Register(Register16 reg, const U16& value);

    void Flag(Flags flag, bool set);
    bool Flag(Flags flag);

    U8 ReadImm8();
    U16 ReadImm16();

    void Push(U8 value);
    void Push(U16 value);
    U8 Push(Register16 reg);
    U8 Pop();
    U16 Pop16();
    U8 Pop(Register16 reg);

    bool Condition(U8 condition);

    U8 Step();
    void SetPC(U16 address);

    template <class... Types>
    static void PrintInstruction(const std::format_string<Types...>& text, Types&&... args);

    inline static std::string ConditionLiteral(U8 cond)
    {
        if (cond == 0x0) return "NZ";
        else if (cond == 0x1) return "Z";
        else if (cond == 0x2) return "NC";
        else if (cond == 0x3) return "C";
        else return "???";
    }

    inline static std::string FlagLiteral(Flags flag)
    {
        return flag == Flags::C ? "C" : flag == Flags::H ? "H" : flag == Flags::N ? "N" : "Z";
    }

    inline static std::string RegisterLiteral(Register8 reg)
    {
        switch (reg)
        {
        case Register8::A: return "A";
        case Register8::B: return "B";
        case Register8::C: return "C";
        case Register8::D: return "D";
        case Register8::E: return "E";
        case Register8::F: return "F";
        case Register8::H: return "H";
        case Register8::L: return "L";
        case Register8::HL: return "HL";
        case Register8::Imm8: return "imm8";
        }

        return "???";
    }

    inline static std::string RegisterLiteral(Register16 reg)
    {
        switch (reg)
        {
        case Register16::AF: return "AF";
        case Register16::BC: return "BC";
        case Register16::DE: return "DE";
        case Register16::HL: return "HL";
        case Register16::SP: return "SP";
        case Register16::PC: return "PC";
        case Register16::HLi: return "[HL+]";
        case Register16::HLd: return "[HL-]";
        case Register16::Imm16: return "imm16";
        }

        return "???";
    }

#pragma region Instructions
    // 16-bit Load Instructions
    U8 LoadFromStackPointer();
    U8 LoadImm16ToR16(Register16 reg);
    U8 LoadAccumulatorToR16Address(Register16 reg);
    U8 LoadR16AddressToAccumulator(Register16 reg);
    U8 LoadSPOffsetToHL();
    U8 LoadHLToSP();

    // 8-bit Load Instructions
    U8 LoadImm8ToR8(Register8 reg);
    U8 LoadR8ToR8(Register8 left, Register8 right);
    U8 LoadHighToAccumulator(Register8 reg);
    U8 LoadHighFromAccumulator(Register8 reg);

    // 8-bit Arithmetic and Logical Instructions
    U8 Add(Register8 reg);
    U8 Adc(Register8 reg);
    U8 Sub(Register8 reg);
    U8 Sbc(Register8 reg);
    U8 And(Register8 reg);
    U8 Xor(Register8 reg);
    U8 Or(Register8 reg);
    U8 Cp(Register8 reg);
    U8 Increment(Register8 reg);
    U8 Decrement(Register8 reg);
    U8 DecimalAdjustAccumulator();
    U8 ComplementAccumulator();
    U8 SetCarryFlag();
    U8 ComplementCarryFlag();

    // 16-bit Arithmetic
    U8 Add(Register16 reg);
    U8 AddSP();
    U8 Increment(Register16 reg);
    U8 Decrement(Register16 reg);

    // Rotate, Shift and Bit Instructions
    U8 RotateLeftCarryAccumulator();
    U8 RotateRightCarryAccumulator();
    U8 RotateLeftAccumulator();
    U8 RotateRightAccumulator();

    U8 RotateLeftCarry(Register8 reg);
    U8 RotateRightCarry(Register8 reg);
    U8 RotateLeft(Register8 reg);
    U8 RotateRight(Register8 reg);

    U8 ShiftLeft(Register8 reg);
    U8 ShiftRight(Register8 reg);
    U8 ShiftRightLogically(Register8 reg);

    U8 Swap(Register8);

    U8 Bit(U8 bitIndex, Register8 reg);
    U8 Reset(U8 bitIndex, Register8 reg);
    U8 Set(U8 bitIndex, Register8 reg);

    // Control Flow Instructions
    U8 Call();
    U8 CallConditional(U8 cond);
    U8 Jump(Register16 reg);
    U8 JumpConditional(U8 cond);
    U8 JumpRelative();
    U8 JumpRelativeConditional(U8 cond);
    U8 Return();
    U8 ReturnI();
    U8 ReturnConditional(U8 cond);
    U8 Restart(U8 vec);

    // Miscellaneous Instructions
    U8 Stop() const;
    U8 DisableInterrupts();
    U8 EnableInterrupts();

#pragma endregion

private:
    std::array<U8, 8> m_Registers;
    std::weak_ptr<Bus> m_Bus;
    std::weak_ptr<LCD> m_LCD;

    static constexpr std::array<Register8, 8> R8 = {
        Register8::B,
        Register8::C,
        Register8::D,
        Register8::E,
        Register8::H,
        Register8::L,
        Register8::HL,
        Register8::A
    };

    static constexpr std::array<Register16, 4> R16 = {
        Register16::BC,
        Register16::DE,
        Register16::HL,
        Register16::SP
    };

    static constexpr std::array<Register16, 4> R16stk = {
        Register16::BC,
        Register16::DE,
        Register16::HL,
        Register16::AF
    };

    static constexpr std::array<Register16, 4> R16mem = {
        Register16::BC,
        Register16::DE,
        Register16::HLi,
        Register16::HLd
    };

    U16 m_SP;
    U16 m_PC;
    bool m_IME;
    bool m_IME_Next_Cycle;
    bool m_Interrupting;
    bool m_Halted;
    U8 m_Wait;
};
