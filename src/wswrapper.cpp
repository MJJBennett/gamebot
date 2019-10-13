#include "wswrapper.hpp"
#include "debug.hpp"
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

namespace websocket = boost::beast::websocket;

web::WSWrapper::~WSWrapper()
{
    qb::log::normal("Closing the websocket.");
    if (ws_ == nullptr)
    {
        // Note - this happens when ws_ is moved from (move constructor)
        // Should probably fix this up to work properly but I'm not sure
        // exactly what the correct steps for that are.
        // Regardless, this works for now.
        return;
    }
    ws_->close(websocket::close_code::normal);
    qb::log::normal("Closed the websocket.");
}

web::WSWrapper::WSWrapper(std::unique_ptr<WebSocket> ws) : ws_(std::move(ws))
{
}

web::WSWrapper::WSWrapper()
{
}

web::WSWrapper::WSWrapper(web::WSWrapper&& wsw) : ws_{std::move(wsw.ws_)}
{
}

web::WSWrapper& web::WSWrapper::operator=(web::WSWrapper&& wsw)
{
    ws_ = std::move(wsw.ws_);
    return *this;
}

void web::WSWrapper::disconnect()
{
    ws_.reset();
}

void web::WSWrapper::validate()
{
    qb::log::normal("Validating WSWrapper.");
    qb::log::normal("Is ws_ null?", (ws_ ? "no" : "yes"));
}
