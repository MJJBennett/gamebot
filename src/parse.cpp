#include "parse.hpp"
#include <algorithm>

std::string qb::parse::remove_non_cmd(std::string str)
{
    return std::string(std::find_if(str.begin(), str.end(), std::isalpha),
                       std::find_if(str.rbegin(), str.rend(), std::isalpha));
}
