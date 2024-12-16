#pragma once
#include <string>

class GameBoyConsole
{
public:
    GameBoyConsole();
    ~GameBoyConsole();
    
    void InsertCartridge(const std::string& cartridge);
    void EjectCartridge();
};
