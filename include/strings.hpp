#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <string>

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

} // namespace endpoints

} // namespace qb

#endif // CONSTANTS_HPP
