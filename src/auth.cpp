#include "auth.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include "constants.hpp"
#include "debug.hpp"
#include "utils.hpp"

void qb::authenticate()
{
    /*
    namespace beast = boost::beast;
    namespace asio  = boost::asio;
    namespace http  = beast::http;
    namespace ssl   = asio::ssl;
    using tcp       = asio::ip::tcp;

    const auto client_id = qb::detail::get_client_id();
    print("Using client id:", '>', client_id, '<');

    // Okay so this kind of defeats the purpose of using string_view constexpr I think
    // However I am under the impression we could make this better, just not sure how right now
    // Really with the current ::detail strategy we could have the full uri at compile time
    const auto target_uri = std::string{qb::constants::oauth_uri} + client_id;
    print("Using request uri:", '>', target_uri, '<');

    // Here's where we start to touch boost abstractions
    asio::io_context ioc;
    tcp::resolver resolver{ioc};
    beast::tcp_stream stream(ioc);

    // Look up the domain name
    auto const results = resolver.resolve(qb::constants::base_url, "80");

    // Make the connection on the IP address we get from a lookup
    stream.connect(results);

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, , 10};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Send the HTTP request to the remote host
    http::write(stream, req);

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::dynamic_body> res;

    // Receive the HTTP response
    http::read(stream, buffer, res);

    // Write the message to standard out
    std::cout << res << std::endl;

    // Gracefully close the socket
    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    */
}
