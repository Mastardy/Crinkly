#include <iostream>

#include "Utility/Utils.hpp"
#include "Hardware/Bus.hpp"

int main(const int argc, char* argv[])
{
    Bus bus;
    bus.Read(0x0000);
    
    return 0;
    
    if (argc != 3) IncorrectUsage(argv[0]);

    std::string action = argv[1];
    std::string filename = argv[2];

    // if (action == "-disasm") Disassembler::Disassemble(filename);
    // else if (action == "-asm") Assembler::Assemble(filename);
    // else if (action == "-run") Interpreter::Run(filename);
    // else IncorrectUsage(argv[0]);
    
    return 0;
}
