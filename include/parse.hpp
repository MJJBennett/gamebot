#ifndef PARSE_HPP
#define PARSE_HPP

#include <string>

namespace qb::parse
{
/** Takes a string and returns the command portion of it. **/
std::string remove_non_cmd(std::string str);

/** Takes a string and returns if it is a stop command. **/
inline bool is_stop(std::string msg)
{
    return remove_non_cmd(std::move(msg)) == "stop";
}

/** Returns whether the first input string starts with the second. **/
inline bool startswith(const std::string& str, const std::string& start)
{
    return ((start.size() <= str.size()) && str.substr(0, start.size()) == start);
}

} // namespace qb::parse

#endif // PARSE_HPP
