#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <string>
#include <vector>

namespace web
{
class EndpointURI : public std::string
{
};
} // namespace web

namespace qb
{
constexpr int http_version = 11; // Technically not a string

namespace strings
{
const std::string port = "443";

} // namespace strings

namespace urls
{
const std::string bot  = "https://discordapp.com/api/v6/gateway/bot";
const std::string base = "discordapp.com";

} // namespace urls

namespace endpoints
{
const std::string invalid   = "";
const std::string websocket = "/?v=6&encoding=json";
const std::string bot       = "/api/v6/gateway/bot";
const auto channel_msg      = [](const std::string& channel_id) {
    return "/api/v6/channels/" + channel_id + "/messages";
};
inline std::string message(const std::string& channel_id, const std::string& message_id)
{
    return "/api/v8/channels/" + channel_id + "/messages/" + message_id;
}
const auto interaction = [](const std::string& specifier) {
    return "/api/v8/interactions" + specifier;
};

inline web::EndpointURI reaction(const std::string& channel_id, const std::string& message_id, const std::string& emoji)
{
    return web::EndpointURI{"/api/v8/channels/" + channel_id + "/messages/" + message_id +
                            "/reactions/" + emoji + "/@me"};
}
} // namespace endpoints

} // namespace qb

#endif // CONSTANTS_HPP
