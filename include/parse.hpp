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

/** Trims a string. **/
std::string trim(std::string str, const std::string& to_trim = " ");
std::string ltrim(std::string str, const std::string& to_trim = " ");
std::string rtrim(std::string str, const std::string& to_trim = " ");

/** Splits a string by the character delimiter. **/
std::vector<std::string> split(const std::string&, char delim = ' ');

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

template <typename Type, typename Range>
bool in(Type t, Range range)
{
    // Currently an inefficient but functional approach.
    for (const auto maybe_t : range)
    {
        if (maybe_t == t) return true;
    }
    return false;
}

/** Returns whether the input string is a potential command. **/
bool is_command(std::string str);

/** Returns the command portion of an input command. **/
std::string get_command(std::string str);

/** Returns command name (and potentially specifier). **/
std::string get_command_name(std::string str);

/** Concatenates strings. **/
inline std::string concatenate(std::vector<std::string> strs, std::string sep = ", ")
{
    if (strs.empty()) return "";
    std::string ret;
    for (auto&& str : strs)
    {
        ret += str + sep;
    }
    return ret.substr(0, ret.size() - sep.size());
}

/** Concatenates strings, but quoted. **/
inline std::string concatenate_quoted(std::vector<std::string> strs, std::string sep = ", ")
{
    if (strs.empty()) return "";
    std::string ret;
    for (auto&& str : strs)
    {
        ret += "\"" + str + "\"" + sep;
    }
    return ret.substr(0, ret.size() - sep.size());
}

} // namespace qb::parse

#endif // PARSE_HPP
