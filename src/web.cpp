#include "web.hpp"
#include "constants.hpp"
#include "debug.hpp"
#include "utils.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <iostream>
#include <string>

namespace beast     = boost::beast;
namespace http      = beast::http;
namespace asio      = boost::asio;
namespace ssl       = asio::ssl;
namespace websocket = beast::websocket;
using tcp           = asio::ip::tcp;

web::WSWrapper::~WSWrapper()
{
    qb::print("Closing the websocket.");
    if (ws_ == nullptr)
    {
        // Note - this happens when ws_ is moved from (move constructor)
        // Should probably fix this up to work properly but I'm not sure
        // exactly what the correct steps for that are.
        // Regardless, this works for now.
        return;
    }
    ws_->close(websocket::close_code::normal);
    qb::print("Closed the websocket.");
}

web::WSWrapper::WSWrapper(std::unique_ptr<WebSocket> ws) : ws_(std::move(ws))
{
}

web::WSWrapper::WSWrapper(web::WSWrapper&& wsw) : ws_{std::move(wsw.ws_)}
{
}

void web::WSWrapper::validate()
{
    qb::print("Validating WSWrapper.");
    qb::print("Is ws_ null?", (ws_ ? "no" : "yes"));
}

web::WSWrapper web::acquire_websocket(const std::string& psocket_url)
{
    const auto port = std::string(qb::constants::port);
    qb::print("Connecting to websocket at URL:", psocket_url, "& port:", port);
    qb::print("Fixing the URL to remove wss://...");
    const auto socket_url = psocket_url.substr(6);
    qb::print("Okay, using", socket_url, "instead.");

    // The io_context is required for all I/O
    asio::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::tlsv12_client};
    ctx.set_options(asio::ssl::context::default_workarounds);
    ctx.set_verify_mode(ssl::verify_peer);

    // These objects perform our I/O
    tcp::resolver resolver{ioc};
    web::WSWrapper ws{std::make_unique<web::WebSocket>(ioc, ctx)};

    qb::print("Resolving websocket URL.");
    auto const results = resolver.resolve(socket_url, port);

    qb::print("Connecting to the IP address using Asio.");
    asio::connect(ws->next_layer().next_layer(), results.begin(), results.end());

    qb::print("Performing SSL handshake.");
    ws->next_layer().handshake(ssl::stream_base::client);

    // Set a decorator to change the User-Agent of the handshake
    ws->set_option(websocket::stream_base::decorator([](websocket::request_type& req) {
        req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-coro");
    }));

    qb::print("Performing the websocket handshake.");
    ws->handshake(socket_url, std::string(qb::constants::websocket_target));

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    // Read a message into our buffer
    qb::print("Reading the Hello payload into the buffer.");
    ws->read(buffer);

    // The make_printable() function helps print a ConstBufferSequence
    std::cout << beast::make_printable(buffer.data()) << std::endl;

    // Send the message
    // ws->write(asio::buffer(std::string("")));

    return std::move(ws);
}

nlohmann::json web::get_bot_socket()
{
    try
    {
        // Step 02 - Set things up to make an API (HTTP) request.
        const std::string host = std::string(qb::constants::host);
        const auto port        = std::string(qb::constants::port);
        const auto target      = std::string(qb::constants::bot_gateway_target);
        const int version      = qb::constants::version;

        // This IO context is a powerful thing, see CPPCon 2018 Falco talk
        asio::io_context ioc;

        tcp::resolver resolver(ioc);

        ssl::context ctx(ssl::context::tlsv12_client);
        ctx.set_options(asio::ssl::context::default_workarounds);
        ctx.set_verify_mode(ssl::verify_peer);

        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.data()))
        {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
            qb::error("Encountered error attempting to set SNI hostname.");
            throw beast::system_error{ec};
        }

        qb::print("Looking up host:", host, "at port:", port);
        const auto results = resolver.resolve(host, port);

        qb::print("Connecting to the IP address.");
        beast::get_lowest_layer(stream).connect(results);

        qb::print("Performing SSL handshake.");
        stream.handshake(ssl::stream_base::client);

        qb::print("Creating an HTTP GET request.");
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::authorization, "Bot " + qb::detail::get_bot_token());

        // Step 03.1 - Send the HTTP request to the remote host
        qb::print("Sending the following HTTP request:");
        qb::print(req);
        http::write(stream, req);

        beast::flat_buffer buffer;             // Useful buffer object
        http::response<http::string_body> res; // Holds response

        // Step 03.2 - Receive the HTTP response
        qb::print("Receiving HTTP response.");
        http::read(stream, buffer, res);

        // Gracefully close the stream
        beast::error_code ec;
        stream.shutdown(ec);
        if (ec == asio::error::eof || ec == ssl::error::stream_truncated)
        {
            qb::print("Ignoring error:", beast::system_error{ec}.what());
            ec = {};
        }
        if (ec) throw beast::system_error{ec};

        // Step 04 - Translate the response to JSON
        return json::parse(res.body());
    }
    catch (const std::exception& e)
    {
        qb::error(e.what());
        return {{"Error", e.what()}};
    }
}
