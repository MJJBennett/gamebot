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
const std::string websocket = "/?v=6&encoding=json";
const std::string bot       = "/api/v6/gateway/bot";
const std::string channel   = "/api/v6/channels/";
const std::string invalid   = "";

} // namespace endpoints

} // namespace qb

#endif // CONSTANTS_HPP
