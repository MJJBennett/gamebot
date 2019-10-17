#ifndef PARSE_HPP
#define PARSE_HPP

#include <nlohmann/json.hpp>
#include <string>

namespace qb::parse
{
bool is_stop(std::string msg)
{
    return remove_non_cmd(std::move(msg)) == "stop";
}

std::string remove_non_cmd(std::string str);
} // namespace qb::parse

#endif // PARSE_HPP
