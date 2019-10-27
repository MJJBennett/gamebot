#include "wswrapper.hpp"
#include "debug.hpp"
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

namespace websocket = boost::beast::websocket;

web::WSWrapper::~WSWrapper()
{
    if (ws_ == nullptr)
    {
        // Note - this happens when ws_ is moved from (move constructor)
        // Should probably fix this up to work properly but I'm not sure
        // exactly what the correct steps for that are.
        // Regardless, this works for now.
        return;
    }
    try
    {
        ws_->close(websocket::close_code::normal);
    }
    catch (const std::exception& e)
    {
        qb::log::err("Caught while closing websocket: ", e.what());
    }
    qb::log::normal("Closed the websocket.");
}

web::WSWrapper::WSWrapper(boost::asio::io_context& ioc)
    : ssl_ctx_(boost::asio::ssl::context::sslv23_client), ioc_(ioc), resolver_(ioc)
{
    qb::log::point("Constructing websocket wrapper.");
    ssl_ctx_.set_options(boost::asio::ssl::context::default_workarounds);
    ssl_ctx_.set_verify_mode(boost::asio::ssl::verify_peer);
    ssl_ctx_.set_default_verify_paths();
    qb::log::point("Constructing inner websocket.");
    ws_ = std::make_unique<web::WebSocket>(ioc_, ssl_ctx_);
}

web::WSWrapper::WSWrapper(web::WSWrapper&& wsw)
    : ws_{std::move(wsw.ws_)}, ssl_ctx_{std::move(wsw.ssl_ctx_)}, ioc_(wsw.ioc_), resolver_{std::move(wsw.resolver_)}
{
}

void web::WSWrapper::disconnect()
{
    try
    {
        ws_->close(websocket::close_code::normal);
    }
    catch (const std::exception& e)
    {
        qb::log::err("Caught while closing websocket: ", e.what());
    }
    ws_.reset();
}
