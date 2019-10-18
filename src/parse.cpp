#include "parse.hpp"

#include <algorithm>
#include <cctype>

std::string qb::parse::remove_non_cmd(std::string str)
{
    auto pred = [](char c) { return std::isalpha(c); };
    return std::string(std::find_if(str.begin(), str.end(), pred),
                       std::find_if(str.rbegin(), str.rend(), pred).base());
}
