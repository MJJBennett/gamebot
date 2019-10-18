#ifndef MESSAGES_HPP
#define MESSAGES_HPP

#include <string>
#include "config.hpp"
#include "parse.hpp"

namespace qb::messages{
inline std::string queue_start(const std::string& cmd)
{
    return "Queuing for games: " + cmd + ". React with :D to do something!";
}
}

#endif // MESSAGES_HPP
