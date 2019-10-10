#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <string>
#include <string_view>

namespace qb::constants
{
using std::string_view_literals::operator""sv;

constexpr auto bot_gateway_url    = "https://discordapp.com/api/v6/gateway/bot"sv;
constexpr auto host               = "discordapp.com"sv;
constexpr int version             = 11;
constexpr auto port               = "443"sv;
constexpr auto bot_gateway_target = "/api/v6/gateway/bot"sv;
constexpr auto websocket_target   = "/?v=6&encoding=json"sv;
} // namespace qb::constants

#endif // CONSTANTS_HPP
