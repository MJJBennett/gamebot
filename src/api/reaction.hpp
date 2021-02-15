#ifndef QB_REACTION_HPP
#define QB_REACTION_HPP

// In the future, we should probably avoid
// having this include brought in. (TODO)
#include <nlohmann/json.hpp>

#include "api.hpp"
#include "emoji.hpp"
#include "user.hpp"
#include "utils/debug.hpp"
#include "utils/json_utils.hpp"
#include "web/strings.hpp"

namespace qb::api
{
using json = nlohmann::json;
struct Reaction
{
    Reaction() = default;
    Reaction(const std::string& message_id,
             const std::string& channel,
             const std::optional<std::string>& guild,
             User user,
             Emoji emoji)
        : message_id(message_id), channel(channel), guild(guild), user(user), emoji(emoji)
    {
    }

    std::string to_string() const
    {
        return "Message ID: " + message_id + "; Channel ID: " + channel + "; User: {" +
               "UNIMPLEMENTED" + "}; Emoji: {" + emoji.to_string() + "};";
    }

    template <bool safe = false>
    static Reaction create(const json& source)
    {
        // https://discord.com/developers/docs/topics/gateway#message-reaction-add
        if constexpr (safe)
        {
            if (source.find("channel_id") == source.end())
            {
                ::qb::log::err("Reaction being constructed from incorrect tier ",
                               "of JSON data (or invalid data):\n", source.dump(2));
                return Reaction{};
            }
        }

        qb::log::point("Creating a reaction.");
        return Reaction(source["message_id"], source["channel_id"],
                        json_utils::get_opt<std::string>(source, "guild_id"),
                        ::qb::api::User(source["user_id"]), ::qb::api::Emoji(source["emoji"]));
    }

    const std::string message_id;
    const std::string channel;
    const std::optional<std::string> guild;
    const User user{};
    const Emoji emoji{};
};
} // namespace qb::api

#endif // QB_REACTION_HPP
