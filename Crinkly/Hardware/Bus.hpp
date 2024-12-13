#pragma once
#include "Utility/Types.hpp"

class Bus
{
public:
    Bus();
    ~Bus();
    
    void Write(Word addr, Byte data);
    Byte Read(Word addr, bool readOnly = false);
};
