#ifndef QB_MESSAGE_HPP
#define QB_MESSAGE_HPP

// In the future, we should probably avoid
// having this include brought in. (TODO)
#include <nlohmann/json.hpp>

#include "api.hpp"
#include "channel.hpp"
#include "user.hpp"
#include "utils/debug.hpp"
#include "utils/json_utils.hpp"
#include "utils/parse.hpp"
#include "web/strings.hpp"

namespace qb::api
{
using json = nlohmann::json;
struct Message
{
    Message() = default;
    Message(const std::string& id, const std::string& channel, const std::string& guild, User user)
        : id(id), channel(channel, guild), user(user)
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
                       json_utils::def_str(source, "guild_id", ""), ::qb::api::User::create(source["author"])

        );
    }

    static std::optional<Message> create_easy(const std::string& source)
    {
        // @id=...;...
        const auto assns = qb::parse::split(source, ';');
        std::string id;
        std::string cid;
        std::string gid;
        std::string uid;
        int sure = 0;

        for (const auto& s : assns)
        {
            const auto assn = qb::parse::split(s, '=');
            if (assn.size() != 2)
            {
                qb::log::warn("Could not parse assignment: ", s);
            }
            else
            {
                sure++;
                if (assn[0] == "id") id = assn[1];
                else if (assn[0] == "cid" || assn[0] == "channel_id") cid = assn[1];
                else if (assn[0] == "gid" || assn[0] == "guild_id") gid = assn[1];
                else if (assn[0] == "uid" || assn[0] == "user_id") uid = assn[1];
                else {
                    qb::log::warn("Could not parse assignment of ", assn[0], " to ", assn[1]);
                    sure--;
                }
            }
        }
        if (sure) {
            qb::log::point("Successfully created a message.");
            return qb::api::Message(id, cid, gid, qb::api::User(uid));
        }
        else return {};
    }

    std::string endpoint() const
    {
        return qb::endpoints::message(channel, id);
    }

    const std::string id;
    const api::Channel channel{};
    const User user{};
};
} // namespace qb::api

#endif // QB_MESSAGE_HPP
