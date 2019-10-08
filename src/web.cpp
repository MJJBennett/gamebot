#include "web.hpp"
#include "constants.hpp"
#include "debug.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <iostream>
#include <string>

nlohmann::json web::get_bot_socket()
{
    namespace beast = boost::beast;
    namespace http  = beast::http;
    namespace asio  = boost::asio;
    namespace ssl   = asio::ssl;
    using tcp       = asio::ip::tcp;

    try
    {
        const std::string host = std::string(qb::constants::host);
        const auto port        = std::string(qb::constants::port);
        const auto target      = std::string(qb::constants::bot_gateway_target);
        const int version      = qb::constants::version;

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
#include "client_tok.hpp" // This is what we call "compile-time I/O"
        req.set(http::field::authorization, "Bot " + __client_tok);

        // Send the HTTP request to the remote host
        qb::print("Sending the HTTP request.");
        std::cout << req << std::endl;
        http::write(stream, req);

        beast::flat_buffer buffer; // Useful buffer object
        http::response<http::string_body> res; // Holds response

        // Receive the HTTP response
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

        return json::parse(res.body());
    }
    catch (const std::exception& e)
    {
        qb::error(e.what());
        return {{"Error", e.what()}};
    }
}
