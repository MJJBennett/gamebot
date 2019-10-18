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

/** Returns the endpoint string for a given enpoint identifier. **/
std::string endpoint_str(Endpoint, const std::string& specifier = "");

class context
{
public:
    context();
    // Initializer, currently must be called to do domain name resolution.
    void initialize();

    ~context();
    // Shuts down the web context (mainly the HTTP stream).
    void shutdown();

    // Returns a wrapped WebSocket connection to the url passed as a parameter.
    [[nodiscard]] WSWrapper acquire_websocket(const std::string& url);

    // Performs an HTTP GET request to the desired Discord endpoint.
    [[nodiscard]] nlohmann::json get(Endpoint);
    // Performs an HTTP POST request to the desired Discord endpoint.
    nlohmann::json post(Endpoint, const std::string& specifier, const std::string& body);

    // Get a pointer to this application's io_context.
    boost::asio::io_context* ioc_ptr()
    {
        return &ioc_;
    }

    // Required to begin asynchronous operations using the io_context.
    void run()
    {
        ioc_.run();
    }

private:
    boost::asio::io_context ioc_{};
    boost::asio::ip::tcp::resolver resolver{ioc_};
    boost::asio::ssl::context ctx_{boost::asio::ssl::context::tlsv12_client};
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream_{ioc_, ctx_};

    bool initialized_{false};
};

} // namespace web

#endif // GAMEBOT_WEB_HPP
