#include "GameBoyConsole.hpp"

#include <fstream>
#include <iostream>

#include "Utility/Types.hpp"
#include "Utility/Utils.hpp"

GameBoyConsole::GameBoyConsole()
{
    std::cout << "GameBoyConsole created.\n";
}

GameBoyConsole::~GameBoyConsole()
{
    EjectCartridge();
    std::cout << "GameBoyConsole destroyed.\n";
}

void GameBoyConsole::InsertCartridge(const std::string& cartridge)
{
    auto splitCartridge = SplitString(cartridge, "\\");
    auto cartridgeName = splitCartridge[splitCartridge.size() - 1];
    std::cout << "Inserting cartridge: " << cartridgeName << '\n';

    std::ifstream input(cartridge, std::ios::binary);
    
    if(input.fail())
    {
        std::cerr << "Failed to open file: " << cartridgeName << '\n';
        std::cin.get();
        exit(127);
    }

    int i = 0;
    while(!input.eof())
    {
        Byte byte;
        input.read(reinterpret_cast<char*>(&byte), sizeof(Byte));
        if (i >= 0x104 && i <= 0x133)
        {
            std::cout << std::hex << static_cast<int>(byte) << '\n';
        }
        i++;
    }
    
    input.close();
}

void GameBoyConsole::EjectCartridge()
{
    std::cout << "Ejecting cartridge.\n";
}