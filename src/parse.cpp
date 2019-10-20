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

std::string qb::parse::trim(std::string str, const std::string& to_trim)
{
    return ltrim(rtrim(str, to_trim));
}

std::string qb::parse::ltrim(std::string str, const std::string& to_trim)
{
    return {std::find_if(str.begin(), str.end(), [to_trim](char c) { return !in(c, to_trim); }), str.end()};
}

std::string qb::parse::rtrim(std::string str, const std::string& to_trim)
{
    return {str.begin(),
            std::find_if(str.rbegin(), str.rend(), [to_trim](char c) { return !in(c, to_trim); }).base()};
}

std::vector<std::string> qb::parse::split(const std::string& str, char delim)
{
    std::vector<std::string> v;
    auto itr = str.begin();
    while (itr != str.end())
    {
        itr = std::find_if_not(itr, str.end(), [delim](char c) { return c == delim; });
        const auto itr_end = std::find_if(itr, str.end(), [delim](char c) { return c == delim; });
        v.push_back({itr, itr_end});
        itr = itr_end;
    }
    return v;
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
