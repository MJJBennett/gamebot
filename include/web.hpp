#ifndef GAMEBOT_WEB_HPP
#define GAMEBOT_WEB_HPP

#include "wswrapper.hpp"
#include <nlohmann/json.hpp>

namespace web
{
enum class Endpoint
{
    channels,
};

nlohmann::json get_bot_socket(boost::asio::io_context& ioc);

// This IO context is a powerful thing, see CPPCon 2018 Falco talk
[[nodiscard]] WSWrapper acquire_websocket(const std::string& url, boost::asio::io_context& ioc);

[[nodiscard]] nlohmann::json get(Endpoint, const std::string& ext);
nlohmann::json post(Endpoint, const std::string& ext, const std::string& body);

const std::string& endpoint_str(Endpoint);

} // namespace web

#endif // GAMEBOT_WEB_HPP
