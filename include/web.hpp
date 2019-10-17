#ifndef GAMEBOT_WEB_HPP
#define GAMEBOT_WEB_HPP

#include "wswrapper.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <nlohmann/json.hpp>

namespace web
{
enum class Endpoint
{
    channels,
    gateway_bot,
};

const std::string& endpoint_str(Endpoint);

class context
{
public:
    context();
    void initialize();
    void shutdown();
    ~context();

    // This IO context is a powerful thing, see CPPCon 2018 Falco talk
    [[nodiscard]] WSWrapper acquire_websocket(const std::string& url);

    [[nodiscard]] nlohmann::json get(Endpoint);
    nlohmann::json post(Endpoint, const std::string& body);

    boost::asio::io_context* ioc_ptr()
    {
        return &ioc_;
    }

    void run()
    {
        ioc_.run();
    }

private:
    boost::asio::io_context ioc_{};
    boost::asio::ip::tcp::resolver resolver{ioc_};
    boost::asio::ssl::context ctx_{boost::asio::ssl::context::tlsv12_client};
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream_{ioc_, ctx_};
};

} // namespace web

#endif // GAMEBOT_WEB_HPP
