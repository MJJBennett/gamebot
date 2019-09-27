#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <string>
#include <string_view>

namespace qb::constants
{
using std::string_view_literals::operator""sv;

constexpr auto base_url  = "www.discordapp.com"sv;
constexpr auto base_uri  = "https://discordapp.com/api/"sv;
constexpr auto oauth_uri = "/api/oauth2/authorize?scope=bot&permissions=1&client_id="sv;
}  // namespace qb::constants

#endif  // CONSTANTS_HPP
