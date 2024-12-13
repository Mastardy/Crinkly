#include "Utils.hpp"

#include <iostream>

void IncorrectUsage(const std::string& exeName)
{
    std::cout << "Incorrect Usage.\nUsage: " << exeName << " <action> <filename>\n";
}
