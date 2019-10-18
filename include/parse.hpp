#ifndef PARSE_HPP
#define PARSE_HPP

#include <string>
#include <vector>

namespace qb::parse
{
/** Takes a string and returns the command portion of it. **/
std::string remove_non_cmd(std::string str);

/** Removes ignored leading characters from a string. **/
std::string trim_leading_ignored(std::string str);

/** Splits a string by the character delimiter. **/
std::vector<std::string> split(const std::string&, char delim=' ');

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

/** Returns whether the first input string starts with the second followed by a non-alphanumeric character. **/
bool startswithword(const std::string& str, const std::string& start);

/** Returns whether the input string is a potential command. **/
bool is_command(std::string str);

/** Returns the command portion of an input command. **/
std::string get_command(std::string str);

} // namespace qb::parse

#endif // PARSE_HPP
