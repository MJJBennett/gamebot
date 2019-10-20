#ifndef MESSAGES_HPP
#define MESSAGES_HPP

#include "config.hpp"
#include "parse.hpp"
#include <string>
#include <vector>

namespace qb::messages
{
inline std::string queue_start(const std::vector<std::string>& cmd)
{
    if (cmd.empty()) return "Not queuing for any games, eh?";
    return "Queuing for games: " + qb::parse::concatenate_quoted(cmd) + "! Reacting does nothing!";
}

inline std::string cannot_store(const std::vector<std::string>& ignored)
{
    return "Error: Cannot store more than " + qb::config::max_skribble_wordsize_s +
           " characters in a single word. The following words were not saved: " +
           qb::parse::concatenate_quoted(ignored);
}

inline std::string did_store(const std::vector<std::string>& ignored)
{
    return "Stored the following words for Skribbl: " + qb::parse::concatenate_quoted(ignored);
}
} // namespace qb::messages

#endif // MESSAGES_HPP
