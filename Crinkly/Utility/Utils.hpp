#pragma once

#include <string>
#include <vector>

#include "Types.hpp"

[[noreturn]]
void IncorrectUsage(const std::string& exeName);

std::vector<std::string> SplitString(const std::string& str, const std::string& delimiter);

static inline constexpr Size operator"" _Kb(unsigned long long n)
{
    return static_cast<Size>(n) * 1024;
}