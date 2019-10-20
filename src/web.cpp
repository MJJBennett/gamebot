#include "web.hpp"

#include "debug.hpp"
#include "strings.hpp"
#include "utils.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

namespace beast     = boost::beast;
namespace http      = beast::http;
namespace asio      = boost::asio;
namespace websocket = beast::websocket;

web::context::context()
{
    ctx_.set_options(asio::ssl::context::default_workarounds);
    ctx_.set_verify_mode(asio::ssl::verify_peer);

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(stream_.native_handle(), qb::urls::base.data()))
    {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
        qb::log::err("Encountered error attempting to set SNI hostname.");
        throw beast::system_error{ec};
    }
}

void web::context::initialize()
{
    qb::log::normal("Looking up host:", qb::urls::base, "at port:", qb::strings::port);
    const auto results = resolver.resolve(qb::urls::base, qb::strings::port);

    qb::log::normal("Connecting to the IP address.");
    beast::get_lowest_layer(stream_).connect(results);

    qb::log::normal("Performing SSL handshake.");
    stream_.handshake(asio::ssl::stream_base::client);

    initialized_ = true;
    qb::log::point("Finished initializing web context.");
}

web::context::~context()
{
    if (initialized_) shutdown();
}

void web::context::shutdown()
{
    initialized_ = false;
    // Properly close the stream to make sure the remote server is aware.
    beast::error_code ec;
    stream_.shutdown(ec);
    if (ec == asio::error::eof || ec == asio::ssl::error::stream_truncated)
    {
        qb::log::normal("Ignoring error:", beast::system_error{ec}.what());
        ec = {};
    }
    if (ec) throw beast::system_error{ec};
}

web::WSWrapper web::context::acquire_websocket(const std::string& psocket_url)
{
    assert(initialized_);

    /** Step 07 - Connect to the socket using websockets and SSL. **/
    qb::log::normal("Connecting to websocket at URL:", psocket_url, "& port:", qb::strings::port);
    qb::log::normal("Fixing the URL to remove wss://...");
    const auto socket_url = psocket_url.substr(6);
    qb::log::normal("Okay, using", socket_url, "instead.");

    // These objects perform our I/O
    web::WSWrapper ws(ioc_);

    qb::log::normal("Resolving websocket URL.");
    auto const results = ws.resolver_.resolve(socket_url, qb::strings::port);

    qb::log::normal("Connecting to the IP address using Asio.");
    asio::connect(ws->next_layer().next_layer(), results.begin(), results.end());

    qb::log::normal("Performing SSL handshake.");
    ws->next_layer().handshake(asio::ssl::stream_base::client);

    // Set a decorator to change the User-Agent of the handshake
    ws->set_option(websocket::stream_base::decorator([](websocket::request_type& req) {
        req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-coro");
    }));

    qb::log::normal("Performing the websocket handshake.");
    ws->handshake(socket_url, std::string(qb::endpoints::websocket));

    return std::move(ws);
}

std::string web::endpoint_str(Endpoint ep, const std::string& specifier)
{
    using EP = web::Endpoint;
    switch (ep)
    {
    case EP::channels:
        return qb::endpoints::channel_msg(specifier);
    case EP::gateway_bot:
        return qb::endpoints::bot;
    default:
        return qb::endpoints::invalid;
    }
}

[[nodiscard]] nlohmann::json web::context::get(Endpoint ep)
{
    assert(initialized_);

    qb::log::normal("Creating an HTTP GET request.");
    http::request<http::string_body> req{http::verb::get, endpoint_str(ep), qb::http_version};
    req.set(http::field::host, qb::urls::base);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::authorization, "Bot " + qb::detail::get_bot_token());

    // Step 03.1 - Send the HTTP request to the remote host
    qb::log::point("Writing an HTTP request.");
    http::write(stream_, req);

    beast::flat_buffer buffer;             // Useful buffer object
    http::response<http::string_body> res; // Holds response

    // Step 03.2 - Receive the HTTP response
    qb::log::normal("Receiving HTTP response.");
    http::read(stream_, buffer, res);

    // Step 04 - Translate the response to JSON
    return nlohmann::json::parse(res.body());
}

nlohmann::json web::context::post(Endpoint ep, const std::string& specifier, const std::string& body)
{
    assert(initialized_);

    qb::log::point("Creating an HTTP POST request.");
    // Set up an HTTP POST request message
    http::request<http::string_body> req{http::verb::post, endpoint_str(ep, specifier), qb::http_version};
    req.set(http::field::host, qb::urls::base);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::authorization, "Bot " + qb::detail::get_bot_token());
    req.body() = body;
    req.set(http::field::content_type, "application/json");
    req.set(http::field::content_length, body.size());
    req.prepare_payload();
    qb::log::data("Sending request...", req);

    // Send the HTTP request to the remote host
    http::write(stream_, req);

    // This buffer is used for reading and must be persisted
    boost::beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::string_body> res;

    // Receive the HTTP response
    qb::log::point("Receiving POST response.");
    try
    {
        http::read(stream_, buffer, res);
    }
    catch (const std::exception& e)
    {
        qb::log::err("Received error: ", e.what());
        throw e;
    }
    return nlohmann::json::parse(res.body());
}
