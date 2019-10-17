#ifndef PARSE_HPP
#define PARSE_HPP

#include <nlohmann/json.hpp>
#include <string>

namespace qb::parse
{
std::string remove_non_cmd(std::string str);

inline bool is_stop(std::string msg)
{
    return remove_non_cmd(std::move(msg)) == "stop";
}
} // namespace qb::parse

#endif // PARSE_HPP
