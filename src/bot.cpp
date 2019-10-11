#include "bot.hpp"
#include "debug.hpp"
#include "json_utils.hpp"
#include "utils.hpp"
#include "web.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <nlohmann/json.hpp>
#include <thread>

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
    qb::log::data("Socket information", socket_info.dump(2));

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
    qb::log::point("Reading the Hello payload into the buffer.");
    ws->read(buffer);

    // The make_printable() function helps print a ConstBufferSequence
    const auto resp = json::parse(beast::buffers_to_string(buffer.data()));
    qb::log::data("Hello payload", resp.dump(2));
    buffer.clear();

    /** Step 09 - Verify that the received OPCODE is equal to 10. **/
    qb::log::normal("Is it opcode 10?", (qb::json_utils::val_eq(resp, "op", 10) ? "Yep!" : "Nope!!!"));
    if (!qb::json_utils::val_eq(resp, "op", 10))
    {
        qb::log::warn("Ending bot execution early.", __LINE__, __FILE__);
        ws.disconnect();
        return;
    }
    ws.validate(); // Print some validation information

    /** Step 10 - Retrieve heartbeat interval information from hello packet. **/
    qb::log::point("Retrieving heartbeat interval.");
    hb_interval_ms_ = resp["d"]["heartbeat_interval"].get<unsigned int>();
    qb::log::value("heartbeat_interval", hb_interval_ms_);

    /** Step 11 - Identify with the remote server. **/

    // Step 12 - Generate the identification packet. (Also Step 13, Step 14 - Adding token/properties)
    const auto identify_packet = qb::json_utils::get_identify_packet(qb::detail::get_bot_token());
    qb::log::data("Identification payload", identify_packet.dump(2));

    qb::log::point("Writing identification payload to websocket.");
    ws->write(asio::buffer(identify_packet.dump()));

    /** Step 15 - Receive Ready packet. **/
    qb::log::point("Reading Ready packet into buffer.");
    ws->read(buffer);

    // The make_printable() function helps print a ConstBufferSequence
    const auto ready_resp = json::parse(beast::buffers_to_string(buffer.data()));
    qb::log::data("Ready payload", ready_resp.dump(2));
    buffer.clear();

    qb::log::normal("Is it opcode 0?", (qb::json_utils::val_eq(ready_resp, "op", 0) ? "Yep!" : "Nope!!!"));
    if (!qb::json_utils::val_eq(ready_resp, "op", 0))
    {
        qb::log::warn("Ending bot execution early", __LINE__, __FILE__);
        ws.disconnect();
        return;
    }

    qb::log::point("Successfully did everything.");
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(5s);

    // For now, just disconnect again, as this is a work in progress.
    qb::log::point("Finishing bot execution...");
}
