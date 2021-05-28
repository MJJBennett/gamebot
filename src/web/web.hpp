#ifndef GAMEBOT_WEB_HPP
#define GAMEBOT_WEB_HPP

#include "wswrapper.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <nlohmann/json.hpp>

// Endpoints
#include "strings.hpp"
#include "api/channel.hpp"
#include "api/interaction.hpp"

namespace web
{
enum class Endpoint
{
    channels,
    gateway_bot,
    interactions,
};

class EndpointURI;

/** Returns the endpoint string for a given enpoint identifier. **/
std::string endpoint_str(Endpoint, const std::string& specifier = "");

class context
{
public:
    enum class Result {
        success,
        retry,
        failure,
    };

    context();
    // Initializer, currently must be called to do domain name resolution.
    void initialize();

    ~context();
    // Shuts down the web context (mainly the HTTP stream).
    void shutdown(bool soft = true);

    // Returns a wrapped WebSocket connection to the url passed as a parameter.
    [[nodiscard]] WSWrapper acquire_websocket(const std::string& url);

    // Performs an HTTP GET request to the desired Discord endpoint.
    [[nodiscard]] nlohmann::json get(Endpoint);
    // Performs an HTTP POST request to the desired Discord endpoint.
    nlohmann::json post(const std::string& specifier, const std::string& body);
    nlohmann::json post(Endpoint, const std::string& specifier, const std::string& body);
    nlohmann::json post(Endpoint, const std::vector<std::string>& specifiers, const std::string& body);
    template<typename T>
    nlohmann::json post(const T& location, const std::string& body) {
        return post(qb::endpoints::of(location), body);
    }

    nlohmann::json put(const EndpointURI& uri, const std::string& body);
    nlohmann::json del(const std::string& uri);
    nlohmann::json patch(const std::string& uri, const std::string& body);
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

    void debug_mode(bool b)
    {
        debug_ = b;
    }

    Result read(boost::beast::flat_buffer& buffer,
                     boost::beast::http::response<boost::beast::http::string_body>& result);

private:
    boost::asio::io_context ioc_{};
    boost::asio::ip::tcp::resolver resolver{ioc_};
    boost::asio::ssl::context ctx_{boost::asio::ssl::context::tlsv12_client};
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream_{ioc_, ctx_};

    bool initialized_{false};
    bool failed_{false};
    bool debug_{false};
};

} // namespace web

#endif // GAMEBOT_WEB_HPP
