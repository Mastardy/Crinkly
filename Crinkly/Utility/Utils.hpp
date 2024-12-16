#pragma once

#include <string>
#include <vector>

[[noreturn]]
void IncorrectUsage(const std::string& exeName);

std::vector<std::string> SplitString(const std::string& str, const std::string& delimiter);