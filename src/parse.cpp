#include "parse.hpp"

#include "config.hpp"
#include <algorithm>
#include <cctype>

using namespace qb;

std::string qb::parse::trim_leading_ignored(std::string str)
{
    return {std::find_if(str.begin(), str.end(), [](char c) { return std::isalpha(c) || c == '!'; }),
            str.end()};
}

std::string qb::parse::remove_non_cmd(std::string str)
{
    auto pred = [](char c) { return std::isalpha(c) || c == '!'; };
    return std::string(std::find_if(str.begin(), str.end(), pred),
                       std::find_if(str.rbegin(), str.rend(), pred).base());
}

bool qb::parse::startswithword(const std::string& str, const std::string& start)
{
    return ((start.size() == str.size()) && str.substr(0, start.size()) == start) ||
           ((start.size() < str.size()) && !isalpha(str[start.size()]) && str.substr(0, start.size()) == start);
}

bool qb::parse::is_command(std::string str)
{
    return startswith(trim_leading_ignored(str), config::cmd_start());
}

std::string qb::parse::get_command(std::string str)
{
    auto cmd_seed = remove_non_cmd(str);
    assert(startswith(cmd_seed, config::cmd_start()));
    return cmd_seed.substr(config::cmd_start().size());
}
