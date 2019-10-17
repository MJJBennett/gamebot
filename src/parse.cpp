#include "parse.hpp"
#include <algorithm>
#include <cctype>

std::string qb::parse::remove_non_cmd(std::string str)
{
    auto pred    = [](char c) { return std::isalpha(c); };
    auto itr     = std::find_if(str.begin(), str.end(), pred);
    auto itr_end = std::find_if(str.rbegin(), str.rend(), pred);
    return std::string(itr, itr_end.base());
}
