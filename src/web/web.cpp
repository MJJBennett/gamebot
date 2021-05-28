#include "web.hpp"

#include "../utils/debug.hpp"
#include "../utils/utils.hpp"
#include "strings.hpp"
#include "utils/json_utils.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/thread.hpp>
#include <numeric>
#include <thread>
namespace beast     = boost::beast;
namespace http      = beast::http;
namespace asio      = boost::asio;
namespace websocket = beast::websocket;

web::context::context()
{
    ctx_.set_options(asio::ssl::context::default_workarounds);
    ctx_.set_verify_mode(asio::ssl::verify_peer);
    ctx_.set_default_verify_paths();

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(stream_.native_handle(), qb::urls::base.data()))
    {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
        qb::log::err("Encountered error attempting to set SNI hostname.");
        throw beast::system_error{ec};
    }

    ctx_.set_default_verify_paths();
}

void web::context::initialize()
{
    qb::log::func("Web Context Initialization: ...Looking up host: ", qb::urls::base, " at port: ", qb::strings::port);
    const auto results = resolver.resolve(qb::urls::base, qb::strings::port);

    qb::log::func(" ...Connecting to the IP address.");
    beast::get_lowest_layer(stream_).connect(results);

    qb::log::func(" ...Performing SSL handshake.");
    stream_.handshake(asio::ssl::stream_base::client);

    initialized_ = true;
    qb::log::func(" ...Finished.\n");
}

web::context::~context()
{
    if (initialized_) shutdown(false);
}

void web::context::shutdown(bool soft)
{
    initialized_ = false;
    // Properly close the stream to make sure the remote server is aware.
    beast::error_code ec;
    stream_.shutdown(ec);
    if (ec == asio::error::eof || ec == asio::ssl::error::stream_truncated || ec == asio::error::broken_pipe)
    {
        qb::log::warn("Ignoring error during web context shutdown: ", beast::system_error{ec}.what());
        ec = {};
    }

    if (!soft && !ioc_.stopped()) ioc_.stop();

    if (ec)
    {
        qb::log::err("Encountered error while shutting down web context stream.");
        throw beast::system_error{ec};
    }
}

web::WSWrapper web::context::acquire_websocket(const std::string& psocket_url)
{
    assert(initialized_);

    /** Step 07 - Connect to the socket using websockets and SSL. **/
    qb::log::func("Context Acquiring Websocket: Connecting to websocket at URL: ", psocket_url, " and port: ", qb::strings::port);
    qb::log::func("\n...Fixing the URL to remove wss://[...]");
    const auto socket_url = psocket_url.substr(6);
    qb::log::func(" ...Okay, using ", socket_url, " instead.");

    // These objects perform our I/O
    web::WSWrapper ws(ioc_);

    qb::log::func(" ...Resolving websocket URL.");
    auto const results = ws.resolver_.resolve(socket_url, qb::strings::port);

    qb::log::func(" ...WebSocket URL: ", socket_url);

    qb::log::func(" ...Connecting to the IP address using Asio.");
    asio::connect(ws->next_layer().next_layer(), results.begin(), results.end());

    qb::log::func(" ...Performing SSL handshake.");
    ws->next_layer().handshake(asio::ssl::stream_base::client);

    // Set a decorator to change the User-Agent of the handshake
    ws->set_option(websocket::stream_base::decorator([](websocket::request_type& req) {
        req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-coro");
    }));

    qb::log::func("\n...Performing the websocket handshake.");
    ws->handshake(socket_url, std::string(qb::endpoints::websocket));
    qb::log::func(" ...WebSocket acquired.\n");

    return ws;
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
    case EP::interactions:
        return qb::endpoints::interaction(specifier);
    default:
        return qb::endpoints::invalid;
    }
}

web::context::Result web::context::read(boost::beast::flat_buffer& buffer,
                     boost::beast::http::response<boost::beast::http::string_body>& result) {
    qb::log::func("web::context::read: Attempting to read from stream.");
    try
    {
        http::read(stream_, buffer, result);
    }
    catch (const std::exception& e)
    {
        qb::log::func("... Errored [", e.what(), "]");
        if (!failed_)
        {
            failed_ = true;
            // It's possible our stream has somehow become disconnected
            // Instead of instantly erroring, let's set the fail check,
            // reinitialize and try again.
            shutdown();
            stream_ = boost::beast::ssl_stream<boost::beast::tcp_stream>{ioc_, ctx_};
            initialize();
            qb::log::func(" ...Retry.\n");
            return Result::retry;
        }
        else
        {
            // Request failed twice in a row, could be a network issue.
            // In the future, this could be a longer timeout.
            qb::log::func(" ...Fully errored (2x):\n");
            qb::log::err(e.what());
            throw e;
        }
    }
    failed_ = false;
    qb::log::func(" ...Success.\n");
    return Result::success;
}

[[nodiscard]] nlohmann::json web::context::get(Endpoint ep)
{
    assert(initialized_);

    qb::log::func("Creating HTTP GET request.");
    http::request<http::string_body> req{http::verb::get, endpoint_str(ep), qb::http_version};
    req.set(http::field::host, qb::urls::base);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::authorization, "Bot " + qb::detail::get_bot_token());

    // Step 03.1 - Send the HTTP request to the remote host
    qb::log::func(" ...Writing the request.");
    http::write(stream_, req);

    beast::flat_buffer buffer;             // Useful buffer object
    http::response<http::string_body> res; // Holds response

    // Step 03.2 - Receive the HTTP response
    qb::log::func(" ...Receiving HTTP response.\n");
    if (read(buffer, res) == Result::retry)
    {
        return get(ep);
    }

    // Step 04 - Translate the response to JSON
    return nlohmann::json::parse(res.body());
}

nlohmann::json web::context::post(Endpoint ep, const std::vector<std::string>& specifiers, const std::string& body)
{
    assert(initialized_);
    return post(ep,
                std::reduce(specifiers.begin(), specifiers.end(), std::string{},
                            [](std::string l, std::string r) { return l + "/" + r; }),
                body);
}

nlohmann::json web::context::post(Endpoint ep, const std::string& specifier, const std::string& body)
{
    assert(initialized_);
    return post(endpoint_str(ep, specifier), body);
}

nlohmann::json web::context::post(const std::string& specifier, const std::string& body)
{
    assert(initialized_);

    qb::log::func("Creating HTTP Post request.");
    // Set up an HTTP POST request message
    http::request<http::string_body> req{http::verb::post, specifier, qb::http_version};
    req.set(http::field::host, qb::urls::base);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::authorization, "Bot " + qb::detail::get_bot_token());
    req.body() = body;
    req.set(http::field::content_type, "application/json");
    req.set(http::field::content_length, std::to_string(body.size()));
    req.prepare_payload();
    if (debug_) {
        qb::log::func(" ...Sending the following request:");
        std::stringstream s;
        s << req;
        qb::log::func("\n", s.str(), "\n");
    }

    // Send the HTTP request to the remote host
    http::write(stream_, req);

    // This buffer is used for reading and must be persisted
    boost::beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::string_body> res;

    // Receive the HTTP response
    qb::log::func(" ...Receiving POST response.");
    if (read(buffer, res) == Result::retry)
    {
        return post(specifier, body);
    }

    // for (auto const& field : res) qb::log::point("field: ", field.name_string(), " | value: ", field.value());

    // If we're using interactions, we don't need the response.
    if (res.find("X-RateLimit-Remaining") == res.end())
    {
        return {};
    }
    if (debug_) qb::log::point("Response body:\n", res.body());

    // The relevant field is: X-RateLimit-Remaining
    qb::log::point("POST Ratelimit remaining: ", res["X-RateLimit-Remaining"]);
    const auto ratelimit_remaining = std::stoi(std::string(res["X-RateLimit-Remaining"]));
    if (ratelimit_remaining == 0)
    {
        qb::log::warn("Hit ratelimit! Self ratelimiting...");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        qb::log::point("Finished self ratelimiting.");
    }

    return qb::json_utils::parse_safe(res.body());
}

nlohmann::json web::context::patch(const std::string& uri, const std::string& body)
{
    assert(initialized_);

    qb::log::func("Creating an HTTP Patch request.");
    http::request<http::string_body> req{http::verb::patch, uri, qb::http_version};
    req.set(http::field::host, qb::urls::base);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::authorization, "Bot " + qb::detail::get_bot_token());
    req.body() = body;
    req.set(http::field::content_type, "application/json");
    req.set(http::field::content_length, std::to_string(body.size()));
    req.prepare_payload();
    if (debug_) {
        qb::log::func(" ...Sending the following request:");
        std::stringstream s;
        s << req;
        qb::log::func("\n", s.str(), "\n");
    }

    // Send the HTTP request to the remote host
    http::write(stream_, req);

    // This buffer is used for reading and must be persisted
    boost::beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::string_body> res;

    // Receive the HTTP response
    qb::log::func(" ...Receiving PATCH response.");
    if (read(buffer, res) == Result::retry)
    {
        return patch(uri, body);
    }

    // for (auto const& field : res) qb::log::point("field: ", field.name_string(), " | value: ", field.value());

    // If we're using interactions, we don't need the response.
    if (debug_) qb::log::point("Response body:\n", res.body());

    // The relevant field is: X-RateLimit-Remaining
    if (res.find("X-RateLimit-Remaining") != res.end())
    {
        qb::log::point("PATCH Ratelimit remaining: ", res["X-RateLimit-Remaining"]);
        const auto ratelimit_remaining = std::stoi(std::string(res["X-RateLimit-Remaining"]));
        if (ratelimit_remaining == 0)
        {
            qb::log::warn("Hit ratelimit! Self ratelimiting...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            qb::log::point("Finished self ratelimiting.");
        }
    }

    return qb::json_utils::parse_safe(res.body());
}
nlohmann::json web::context::put(const EndpointURI& uri, const std::string& body)
{
    assert(initialized_);

    qb::log::scope<std::string> slg("[HTTP PUT request creation start.]");
    // Set up an HTTP POST request message
    http::request<http::string_body> req{http::verb::put, uri, qb::http_version};
    req.set(http::field::host, qb::urls::base);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::authorization, "Bot " + qb::detail::get_bot_token());
    req.body() = body;
    req.set(http::field::content_type, "application/json");
    req.set(http::field::content_length, std::to_string(body.size()));
    req.prepare_payload();
    slg += "\nSending the following request:\n";
    std::stringstream s;
    s << req;
    slg += s.str();

    // Send the HTTP request to the remote host
    http::write(stream_, req);

    // This buffer is used for reading and must be persisted
    boost::beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::string_body> res;

    // Receive the HTTP response
    slg += ("\nReceiving PUT response.");
    try
    {
        http::read(stream_, buffer, res);
    }
    catch (const std::exception& e)
    {
        qb::log::warn("Got error: ", e.what(), " while reading the response to a POST request.");
        if (!failed_)
        {
            if (!debug_) slg.clear();
            failed_ = true;
            // It's possible our stream has somehow become disconnected
            // Instead of instantly erroring, let's set the fail check,
            // reinitialize and try again.
            shutdown();
            stream_ = boost::beast::ssl_stream<boost::beast::tcp_stream>{ioc_, ctx_};
            initialize();
            return put(uri, body);
        }
        else
        {
            // Request failed twice in a row, could be a network issue.
            // In the future, this could be a longer timeout.
            throw e;
        }
    }
    failed_ = false;
    if (!debug_) slg.clear();

    // for (auto const& field : res) qb::log::point("field: ", field.name_string(), " | value: ", field.value());

    // If we're using interactions, we don't need the response.
    if (debug_) qb::log::point("Response body:\n", res.body());

    // The relevant field is: X-RateLimit-Remaining
    if (res.find("X-RateLimit-Remaining") != res.end())
    {
        qb::log::point("PUT Ratelimit remaining: ", res["X-RateLimit-Remaining"]);
        const auto ratelimit_remaining = std::stoi(std::string(res["X-RateLimit-Remaining"]));
        if (ratelimit_remaining == 0)
        {
            qb::log::warn("Hit ratelimit! Self ratelimiting...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            qb::log::point("Finished self ratelimiting.");
        }
    }

    return qb::json_utils::parse_safe(res.body());
}

nlohmann::json web::context::del(const std::string& uri)
{
    assert(initialized_);

    qb::log::scope<std::string> slg("[HTTP DELETE request creation start.]");
    // Set up an HTTP POST request message
    http::request<http::string_body> req{http::verb::delete_, uri, qb::http_version};
    req.set(http::field::host, qb::urls::base);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::authorization, "Bot " + qb::detail::get_bot_token());
    req.body() = "";
    req.set(http::field::content_type, "application/json");
    req.set(http::field::content_length, std::to_string(0));
    req.prepare_payload();
    slg += "\nSending the following request:\n";
    std::stringstream s;
    s << req;
    slg += s.str();

    // Send the HTTP request to the remote host
    http::write(stream_, req);

    // This buffer is used for reading and must be persisted
    boost::beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::string_body> res;

    // Receive the HTTP response
    slg += ("\nReceiving POST response.");
    try
    {
        http::read(stream_, buffer, res);
    }
    catch (const std::exception& e)
    {
        qb::log::warn("Got error: ", e.what(), " while reading the response to a POST request.");
        if (!failed_)
        {
            if (!debug_) slg.clear();
            failed_ = true;
            // It's possible our stream has somehow become disconnected
            // Instead of instantly erroring, let's set the fail check,
            // reinitialize and try again.
            shutdown();
            stream_ = boost::beast::ssl_stream<boost::beast::tcp_stream>{ioc_, ctx_};
            initialize();
            return del(uri);
        }
        else
        {
            // Request failed twice in a row, could be a network issue.
            // In the future, this could be a longer timeout.
            throw e;
        }
    }
    failed_ = false;
    if (!debug_) slg.clear();

    // for (auto const& field : res) qb::log::point("field: ", field.name_string(), " | value: ", field.value());

    // If we're using interactions, we don't need the response.
    if (debug_) qb::log::point("Response body:\n", res.body());

    // The relevant field is: X-RateLimit-Remaining
    if (res.find("X-RateLimit-Remaining") != res.end())
    {
        qb::log::point("DELETE Ratelimit remaining: ", res["X-RateLimit-Remaining"]);
        const auto ratelimit_remaining = std::stoi(std::string(res["X-RateLimit-Remaining"]));
        if (ratelimit_remaining == 0)
        {
            qb::log::warn("Hit ratelimit! Self ratelimiting...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            qb::log::point("Finished self ratelimiting.");
        }
    }

    return qb::json_utils::parse_safe(res.body());
}
