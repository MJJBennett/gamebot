#ifndef QB_CHANNEL_HPP
#define QB_CHANNEL_HPP

#include "web/strings.hpp"

namespace qb::api
{
struct Channel
{
    Channel() = default;
    Channel(const std::string& id, const std::string& guild) : id(id), guild(guild)
    {
    }

    const std::string id;
    const std::string guild;
};
} // namespace qb::api

namespace qb::endpoints
{
inline std::string message(const qb::api::Channel& channel, const std::string& message_id)
{
    return ::qb::endpoints::message(channel.id, message_id);
}

inline web::EndpointURI reaction(const qb::api::Channel& channel, const std::string& message_id, const std::string& emoji)
{
    return web::EndpointURI{"/api/v8/channels/" + channel.id + "/messages/" + message_id +
                            "/reactions/" + emoji + "/@me"};
};
} // namespace qb::endpoints

#endif // QB_CHANNEL_HPP
