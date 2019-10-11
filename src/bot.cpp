#include "bot.hpp"
#include "debug.hpp"
#include "json_utils.hpp"
#include "utils.hpp"
#include "web.hpp"
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <nlohmann/json.hpp>
#include <boost/beast/core.hpp>

namespace beast     = boost::beast;
namespace http      = beast::http;
namespace asio      = boost::asio;
namespace ssl       = asio::ssl;
namespace websocket = beast::websocket;
using tcp           = asio::ip::tcp;
using json          = nlohmann::json;

qb::Bot::Bot(const Flag flag)
{
    using namespace qb;
    if (bitwise_and<int>(flag, Flag::LazyInit)) return;
    start();
}

void qb::Bot::start()
{
    // Step 01 - Make API call to Discord /gateway/bot/ to get a WebSocket URL
    // This is something we might need to do intermittently, so we call a function to do it.
    auto socket_info = web::get_bot_socket();
    qb::log::data("Socket information", socket_info.dump(4));

    // Step 05 - Get the socket URL from the JSON data and save it with explicit version/encoding
    const std::string socket_url = socket_info["url"];
    // This isn't particularly efficient but optimizing startup time doesn't seem like a priority

    // Step 06 - Start running bot core.

    // This requires a connection to the remote WebSocket server.
    // We will use our abstractions in web.hpp to acquire this for us.
    auto ws = web::acquire_websocket(socket_url);

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    /** Step 08 - Receive OPCODE 10 packet. **/
    qb::log::normal("Reading the Hello payload into the buffer.");
    ws->read(buffer);

    // The make_printable() function helps print a ConstBufferSequence
    const auto resp = json::parse(beast::buffers_to_string(buffer.data()));
    // qb::log::normal(beast::make_printable(buffer.data()));

    /** Step 09 - Verify that the received OPCODE is equal to 10. **/
    qb::log::normal("Is it opcode 10?", (qb::json_utils::val_eq(resp, "op", 10) ? "Yep!" : "Nope!!!"));
    if (!qb::json_utils::val_eq(resp, "op", 10))
    {
        qb::log::point("Ending bot execution early.");
        ws.disconnect();
        return;
    }
    ws.validate(); // Print some validation information

    /** Step 10 - Retrieve heartbeat interval information from hello packet. **/
    // Send the message
    // ws->write(asio::buffer(std::string("")));

    // For now, just disconnect again, as this is a work in progress.
    qb::log::point("Finishing bot execution...");
}
