#include <iostream>

#include "GameBoyConsole.hpp"

#include "Utility/Utils.hpp"

int main(const int argc, char* argv[])
{
    GameBoyConsole console;
    
    if (argc != 2) IncorrectUsage(argv[0]);

    std::string filename = argv[1];

    console.InsertCartridge(filename);

    return 0;

    // if (argc != 3) IncorrectUsage(argv[0]);

    // std::string action = argv[1];
    // std::string filename = argv[2];

    // if (action == "-disasm") Disassembler::Disassemble(filename);
    // else if (action == "-asm") Assembler::Assemble(filename);
    // else if (action == "-run") Interpreter::Run(filename);
    // else IncorrectUsage(argv[0]);
}
