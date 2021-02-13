#ifndef QB_MESSAGE_HPP
#define QB_MESSAGE_HPP

// In the future, we should probably avoid
// having this include brought in. (TODO)
#include <nlohmann/json.hpp>

#include "utils/debug.hpp"
#include "utils/json_utils.hpp"

namespace qb::api
{
using json = nlohmann::json;
class Message
{
    Message() = default;
    Message(const std::string& id, const std::string& channel, const std::string& guild)
        : id(id), channel(channel), guild(guild)
    {
    }

public:
    template <bool safe = false>
    static Message create(const json& source)
    {
        // The json here should be the full data of the message.
        // However, it's possible that a developer allows the wrong
        // tier of JSON to propagate here. If we're running in safe
        // mode, we check for that.
        if constexpr (safe)
        {
            if (source.find("embed") == source.end())
            {
                ::qb::log::err("Message being constructed from incorrect tier ",
                               "of JSON data (or invalid data):\n", source.dump(2));
                return Message{};
            }
        }

        return Message(source["id"], source["channel_id"],
                       json_utils::def_str(source, "guild_id", ""));
    }

public:
    // Make these public; there's no reason to have getters/setters.
    std::string id;
    std::string channel;
    std::string guild;
};
} // namespace qb::api

#endif // QB_MESSAGE_HPP
