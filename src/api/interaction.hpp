#ifndef QB_INTERACTION_HPP
#define QB_INTERACTION_HPP

// In the future, we should probably avoid
// having this include brought in. (TODO)
#include <nlohmann/json.hpp>

#include "api.hpp"
#include "message.hpp"
#include "user.hpp"
#include "utils/debug.hpp"
#include "utils/json_utils.hpp"
#include "web/strings.hpp"

namespace qb::api
{
using json = nlohmann::json;
struct Interaction
{
    Interaction() = default;
    Interaction(const std::string& id,
                const std::string& token,
                const User& user,
                const std::optional<api::Channel>& channel,
                const std::optional<api::Message>& message)
        : id(id), token(token), user(user), channel(channel), message(message)
    {
    }

    std::string key() const
    {
        return (message ? (*message).id : "IDK_SLASH_COMMAND");
    }

    std::string to_string() const
    {
        return "nothing right now for this :)";
    }

    template <bool safe = false>
    static Interaction create(const json& src)
    {
        // https://discord.com/developers/docs/interactions/slash-commands#interaction
        if constexpr (safe)
        {
            if (src.find("channel_id") == src.end())
            {
                ::qb::log::err("Interaction being constructed from incorrect tier ",
                               "of JSON data (or invalid data):\n", src.dump(2));
                return Interaction{};
            }
        }

        const auto ujson = src.contains("user") ? src["user"] : src["member"]["user"];

        qb::log::point("Creating an interaction.");
        return api::Interaction(
            src["id"], src["token"], api::User::create(ujson),
            (src.contains("channel_id") ? api::Channel(src["channel_id"], src["guild_id"])
                                        : std::optional<api::Channel>{}),
            (src.contains("message") ? api::Message::create(src["message"]) : std::optional<api::Message>{}));
    }

    const std::string id;
    const std::string token;
    const User user{};

    const std::optional<api::Channel> channel;
    const std::optional<api::Message> message;
};
} // namespace qb::api

#endif // QB_INTERACTION_HPP
