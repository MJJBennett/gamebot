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
struct ApplicationCommandInteractionData
{
    static ApplicationCommandInteractionData create(const nlohmann::json& src)
    {
        if (src.empty()) return {};
        qb::log::point("Creating ApplicationCommandInteractionData from ", src.dump());
        return ApplicationCommandInteractionData{src.value("id", ""), src.value("name", ""),
                                                 src.value("custom_id", ""),
                                                 src.value("component_type", -1)};
    }
    std::string to_string() const
    {
        return "ApplicationCommandInteractionData{" + id + ", " + name + ", " + custom_id + ", " +
               std::to_string(component_type) + "}";
    }
    const std::string id;
    const std::string name;
    const std::string custom_id;
    const int component_type;
};

struct InteractionResponse
{
    InteractionResponse(int type = 4) : type(type)
    {
    }

    nlohmann::json as_json()
    {
        nlohmann::json resp{{"type", type}};
        if (content || flags)
        {
            resp["data"] = nlohmann::json::object();
        }
        if (content)
        {
            resp["data"]["content"] = *content;
        }
        if (flags)
        {
            resp["data"]["flags"] = flags;
        }
        return resp;
    }

    InteractionResponse& with_type(int val)
    {
        type = val;
        return *this;
    }

    InteractionResponse& with_content(std::string val)
    {
        content = std::move(val);
        return *this;
    }

    InteractionResponse& with_flags(int val)
    {
        flags |= val;
        return *this;
    }

    int type = 4;
    std::optional<std::string> content;
    int flags = 0;
};

struct Interaction
{
    Interaction() = default;
    Interaction(const std::string& id,
                const std::string& token,
                const User& user,
                const ApplicationCommandInteractionData& data,
                const std::optional<api::Channel>& channel,
                const std::optional<api::Message>& message)
        : id(id), token(token), user(user), data(data), channel(channel), message(message)
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
        const auto djson = src.contains("data") ? src["data"] : nlohmann::json{};
        using dtype      = ApplicationCommandInteractionData;

        qb::log::point("Creating an interaction.");
        return api::Interaction(
            src["id"], src["token"], api::User::create(ujson), dtype::create(djson),
            (src.contains("channel_id") ? api::Channel(src["channel_id"], src["guild_id"])
                                        : std::optional<api::Channel>{}),
            (src.contains("message") ? api::Message::create(src["message"]) : std::optional<api::Message>{}));
    }

    static InteractionResponse response(int type = 4)
    {
        return InteractionResponse(type);
    }

    const std::string id;
    const std::string token;
    const User user{};
    const ApplicationCommandInteractionData data{};

    const std::optional<api::Channel> channel;
    const std::optional<api::Message> message;
};
} // namespace qb::api

namespace qb
{
inline std::string interaction(const qb::api::Interaction& interaction)
{
    return "/api/v8/interactions/" + interaction.id + "/" + interaction.token + "/callback";
}

namespace endpoints
{
inline std::string of(const qb::api::Interaction& i)
{
    return ::qb::interaction(i);
}
} // namespace endpoints
} // namespace qb
#endif // QB_INTERACTION_HPP
