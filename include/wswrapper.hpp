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
    WSWrapper(boost::asio::io_context& ioc);
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

    boost::asio::ip::tcp::resolver resolver_;
private:
    std::unique_ptr<WebSocket> ws_;
    boost::asio::ssl::context ctx_;
    boost::asio::io_context& ioc_;
};
} // namespace web

#endif // WSWRAPPER_HPP
