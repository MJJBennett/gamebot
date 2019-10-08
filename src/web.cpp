#include "web.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <string>
#include "constants.hpp"
#include <iostream>
#include "debug.hpp"

nlohmann::json web::get_bot_socket()
{
    namespace beast = boost::beast; // from <boost/beast.hpp>
    namespace http = beast::http;   // from <boost/beast/http.hpp>
    namespace asio = boost::asio;    // from <boost/asio.hpp>
    namespace ssl = asio::ssl;       // from <boost/asio/ssl.hpp>
    using tcp = asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

    try
    {
       const std::string host = std::string(qb::constants::host);
        auto const port = std::string(qb::constants::port);
        auto const target = std::string(qb::constants::bot_gateway_target);
        int version = qb::constants::version;

        // The io_context is required for all I/O
        asio::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx(ssl::context::tlsv12_client);

        ctx.set_options(
            asio::ssl::context::default_workarounds);

        // Verify the remote server's certificate
        ctx.set_verify_mode(ssl::verify_peer);

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if(! SSL_set_tlsext_host_name(stream.native_handle(), host.data()))
        {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
            throw beast::system_error{ec};
        }

        // Look up the domain name
        qb::print("Looking up host:", host, "at port:", port);
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        qb::print("Connecting to the IP address.");
        beast::get_lowest_layer(stream).connect(results);

        // Perform the SSL handshake
        qb::print("Performing SSL handshake.");
        stream.handshake(ssl::stream_base::client);

        // Set up an HTTP GET request message
        qb::print("Creating an HTTP GET request.");
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
#include "client_tok.hpp"
        req.set(http::field::authorization, "Bot " + __client_tok);

        // Send the HTTP request to the remote host
        qb::print("Sending the HTTP request.");
        std::cout << req << std::endl;
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::string_body> res;

        // Receive the HTTP response
        qb::print("Receiving HTTP response.");
        http::read(stream, buffer, res);

        // Write the message to standard out
        std::cout << res << std::endl;

        // Gracefully close the stream
        beast::error_code ec;
        stream.shutdown(ec);
        if(ec == asio::error::eof)
        {
            qb::print("ASIO EOF!!");
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec = {};
        }
        else if (ec == ssl::error::stream_truncated)
        {
            qb::print("IGNORING STREAM TRUNCATION OBVS");
            ec = {};
        }
        if(ec)
            throw beast::system_error{ec};

        qb::print("We are gucci fam");
        std::cout << res.body() << std::endl;

        // If we get here then the connection is closed gracefully
        return json::parse(res.body());
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return {};
    }

    return {};
}
