#include "Utils.hpp"

#include <iostream>

void IncorrectUsage(const std::string& exeName)
{
    std::cout << "Incorrect Usage.\nUsage: " << exeName << " <filename>\n";

    system("pause");
    exit(127);
}

std::vector<std::string> SplitString(const std::string& str, const std::string& delimiter)
{
    std::vector<std::string> result;

    auto tempStr = str;
    auto pos = tempStr.find(delimiter);

    while (pos != std::string::npos)
    {
        result.push_back(tempStr.substr(0, pos));
        tempStr = tempStr.substr(pos + delimiter.size());
        pos = tempStr.find(delimiter);
    }

    result.push_back(tempStr);

    return result;
}