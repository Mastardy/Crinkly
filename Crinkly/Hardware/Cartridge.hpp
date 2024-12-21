#pragma once
#include <string>
#include <vector>

#include "Utility/Types.hpp"

class Cartridge
{
public:
    Cartridge(const std::string& file);

    Cartridge(const Cartridge&) = delete;
    Cartridge& operator=(const Cartridge&) = delete;
    Cartridge(Cartridge&&) = delete;
    Cartridge& operator=(Cartridge&&) = delete;

private:
    std::vector<Byte> m_ROM;
};
