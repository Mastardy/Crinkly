#include <iostream>

#include "GameBoyConsole.hpp"

#include "Utility/Utils.hpp"

int main(const int argc, char* argv[])
{
    GameBoyConsole console;

    if (argc != 2) IncorrectUsage(argv[0]);

    std::string filename = argv[1];

    console.InsertCartridge(filename);

    std::cin.get();

    return 0;
}
