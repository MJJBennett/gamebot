#ifndef GAMEBOT_WEB_HPP
#define GAMEBOT_WEB_HPP

#include "wswrapper.hpp"
#include <nlohmann/json.hpp>

namespace web
{
nlohmann::json get_bot_socket();

[[nodiscard]] WSWrapper acquire_websocket(const std::string& url);

} // namespace web

#endif // GAMEBOT_WEB_HPP
