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
    std::string game_msg;
    for (auto&& str : cmd)
    {
        game_msg += "\"" + str + "\", ";
    }
    return "Queuing for games: " + game_msg.substr(0, game_msg.size() - 2) +
           "! Reacting does nothing!";
}
} // namespace qb::messages

#endif // MESSAGES_HPP
