#pragma once

#include <functional>

#include "Utility/Types.hpp"

class Joypad
{
public:
    enum class Button : U8
    {
        Right = 0x00,
        Left = 0x01,
        Up = 0x02,
        Down = 0x03,

        A = 0x00,
        B = 0x01,
        Select = 0x02,
        Start = 0x03
    };

    Joypad();

    void SetRequestJoypadInterruptCallback(std::function<void()> callback);

    U8 Read() const;
    void Write(U8 value);

    void SetDPad(Button button, bool pressed);
    void SetButtons(Button button, bool pressed);

private:
    void SetLine(U8& group, bool selected, Button button, bool pressed);

    U8 m_Select;
    U8 m_DPad;
    U8 m_Buttons;

    std::function<void()> m_RequestJoypadInterrupt;
};
