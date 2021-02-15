#ifndef PARSE_HPP
#define PARSE_HPP

#include <string>
#include <tuple>
#include <vector>
#include <optional>

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

std::tuple<std::string, std::vector<std::string>> get_time(std::vector<std::string>);
std::tuple<std::string, std::vector<std::string>> match(std::string, std::vector<std::string>);
bool match(std::string, std::string);

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

inline std::optional<std::string> emote_snowflake(const std::string& full_emote) {
    if (full_emote.size() == 0) return {};
    if (full_emote[0] & 0x10000000) return full_emote;
    auto sf_begin = std::find_if(full_emote.rbegin(), full_emote.rend(), isdigit);    
    if (sf_begin == full_emote.rend()) return {};
    auto sf_end = std::find_if(sf_begin, full_emote.rend(), [](char c) {return !isdigit(c);});
    const auto res = std::string{sf_end.base(), sf_begin.base()};
    if (res.size() < 4) return {};
    // now we need the name, apparently
    const auto name = std::string{full_emote.begin() + 2, sf_end.base()};
    return name + res;
}

} // namespace qb::parse

#endif // PARSE_HPP
