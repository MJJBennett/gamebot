#ifndef GAMEBOT_WEB_HPP
#define GAMEBOT_WEB_HPP

#include "boost/asio/ip/tcp.hpp"
#include "boost/beast/ssl/ssl_stream.hpp"
#include "boost/beast/websocket/stream_fwd.hpp"
#include "nlohmann/json.hpp"
#include <memory>

namespace web
{
// Types / Aliases
using json = nlohmann::json;
using WebSocket = boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>;

// Class definitions
class WSWrapper
{
public:
    WSWrapper() = delete;
    WSWrapper(std::unique_ptr<WebSocket> ws);
    ~WSWrapper();
    WSWrapper(WSWrapper&& wsw);
    WebSocket* operator->()
    {
        return ws_.get();
    }
    void validate();

private:
    std::unique_ptr<WebSocket> ws_;
};

json get_bot_socket();

WSWrapper acquire_websocket(const std::string& url);

} // namespace web

#endif // GAMEBOT_WEB_HPP
