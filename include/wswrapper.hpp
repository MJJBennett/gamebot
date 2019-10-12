#ifndef WSWRAPPER_HPP
#define WSWRAPPER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/websocket/stream_fwd.hpp>
#include <memory>

namespace web
{
using WebSocket = boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>;

class WSWrapper
{
public:
    WSWrapper();
    WSWrapper(std::unique_ptr<WebSocket> ws);
    ~WSWrapper();
    WSWrapper(WSWrapper&& wsw);
    WebSocket* operator->()
    {
        return ws_.get();
    }
    void validate();
    bool operator==(const WSWrapper& other)
    {
        return other.ws_.get() == ws_.get();
    }
    void disconnect();

private:
    std::unique_ptr<WebSocket> ws_;
};
} // namespace web

#endif // WSWRAPPER_HPP
