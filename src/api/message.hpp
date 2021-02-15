#ifndef QB_MESSAGE_HPP
#define QB_MESSAGE_HPP

// In the future, we should probably avoid
// having this include brought in. (TODO)
#include <nlohmann/json.hpp>

#include "api.hpp"
#include "utils/debug.hpp"
#include "utils/json_utils.hpp"
#include "user.hpp"
#include "web/strings.hpp"

namespace qb::api
{
using json = nlohmann::json;
struct Message
{
    Message() = default;
    Message(const std::string& id, const std::string& channel, const std::string& guild, User user)
        : id(id), channel(channel), guild(guild), user(user)
    {
    }

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
                       json_utils::def_str(source, "guild_id", ""),
                       ::qb::api::User::create(source["author"])

                        );
    }

    std::string endpoint() const
    {
        return qb::endpoints::message(channel, id);
    }

    const std::string id;
    const std::string channel;
    const std::string guild;
    const User user{};
};
} // namespace qb::api

#endif // QB_MESSAGE_HPP
