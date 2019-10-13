#include "bot.hpp"
#include "debug.hpp"
#include "json_utils.hpp"
#include "utils.hpp"
#include "web.hpp"
#include <boost/asio/steady_timer.hpp>
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

void qb::Bot::ping_sender(const boost::system::error_code& error)
{
    // Check if we've already fired an async_write
    if (outstanding_write_)
    {
        // Pings aren't that important, try again in a second
        qb::log::point("Outstanding write, skipping ping for a second.");
        boost::asio::steady_timer short_timer(
            ioc_, std::chrono::steady_clock::now() + std::chrono::seconds(1));
        short_timer.async_wait(std::bind(&qb::Bot::ping_sender, this, std::placeholders::_1));
        return;
    }
    // Send a ping
    const static auto heartbeat_msg =
        asio::buffer(json{{"op", 1}, {"s", nullptr}, {"d", {}}, {"t", nullptr}}.dump());
    qb::log::point("Sending ping.");
    ws_->async_write(heartbeat_msg, std::bind(&qb::Bot::write_complete_handler, this,
                                              std::placeholders::_1, std::placeholders::_2));
    outstanding_write_ = true;
    boost::asio::steady_timer timer(
        ioc_, std::chrono::steady_clock::now() + std::chrono::milliseconds(hb_interval_ms_));
    timer.async_wait(std::bind(&qb::Bot::ping_sender, this, std::placeholders::_1));
}

void qb::Bot::write_complete_handler(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    // Our most recent write is now complete. That's great!
    outstanding_write_ = false;
}

void qb::Bot::read_handler(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    qb::log::data("Data read from websocket", json::parse(beast::buffers_to_string(buffer_.data())));
    buffer_.consume(buffer_.size());
    ws_->async_read(buffer_, std::bind(&qb::Bot::read_handler, this, std::placeholders::_1,
                                              std::placeholders::_2));
}

void qb::Bot::start()
{
    // Step 01 - Make API call to Discord /gateway/bot/ to get a WebSocket URL
    // This is something we might need to do intermittently, so we call a function to do it.
    auto socket_info = web::get_bot_socket(ioc_);
    qb::log::data("Socket information", socket_info.dump(2));

    // Step 05 - Get the socket URL from the JSON data and save it with explicit version/encoding
    const std::string socket_url = socket_info["url"];
    // This isn't particularly efficient but optimizing startup time doesn't seem like a priority

    // Step 06 - Start running bot core.

    // This requires a connection to the remote WebSocket server.
    // We will use our abstractions in web.hpp to acquire this for us.
    ws_ = std::move(web::acquire_websocket(socket_url));

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    /** Step 08 - Receive OPCODE 10 packet. **/
    qb::log::point("Reading the Hello payload into the buffer.");
    ws_->read(buffer);

    // The make_printable() function helps print a ConstBufferSequence
    const auto resp = json::parse(beast::buffers_to_string(buffer.data()));
    qb::log::data("Hello payload", resp.dump(2));
    buffer.consume(buffer.size());

    /** Step 09 - Verify that the received OPCODE is equal to 10. **/
    qb::log::normal("Is it opcode 10?", (qb::json_utils::val_eq(resp, "op", 10) ? "Yep!" : "Nope!!!"));
    if (!qb::json_utils::val_eq(resp, "op", 10))
    {
        qb::log::warn("Ending bot execution early. @", __LINE__, __FILE__);
        ws_.disconnect();
        return;
    }
    ws_.validate(); // Print some validation information

    /** Step 10 - Retrieve heartbeat interval information from hello packet. **/
    qb::log::point("Retrieving heartbeat interval.");
    hb_interval_ms_ = resp["d"]["heartbeat_interval"].get<unsigned int>();
    qb::log::value("heartbeat_interval", hb_interval_ms_);

    /** Step 11 - Identify with the remote server. **/

    // Step 12 - Generate the identification packet. (Also Step 13, Step 14 - Adding token/properties)
    const auto identify_packet = qb::json_utils::get_identify_packet(qb::detail::get_bot_token());
    qb::log::data("Identification payload", identify_packet.dump(2));

    qb::log::point("Writing identification payload to websocket.");
    ws_->write(asio::buffer(identify_packet.dump()));

    /** Step 15 - Receive Ready packet. **/
    qb::log::point("Reading Ready packet into buffer.");
    ws_->read(buffer);

    // The make_printable() function helps print a ConstBufferSequence
    const auto ready_resp = json::parse(beast::buffers_to_string(buffer.data()));
    qb::log::data("Ready payload", ready_resp.dump(2));
    buffer.consume(buffer.size());

    qb::log::normal("Is it opcode 0?", (qb::json_utils::val_eq(ready_resp, "op", 0) ? "Yep!" : "Nope!!!"));
    if (!qb::json_utils::val_eq(ready_resp, "op", 0))
    {
        qb::log::warn("Ending bot execution early. @", __LINE__, __FILE__);
        ws_.disconnect();
        return;
    }
    qb::log::point("Successfully completed setup. Now beginning normal asynchronous operations.");

    ws_->async_read(buffer_, std::bind(&qb::Bot::read_handler, this, std::placeholders::_1,
                                              std::placeholders::_2));
    ping_sender({});
    ioc_.run();

    // using namespace std::chrono_literals;
    // std::this_thread::sleep_for(5s);

    // For now, just disconnect again, as this is a work in progress.
    qb::log::point("Finishing bot execution...");
}
