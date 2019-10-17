#include "bot.hpp"
#include "debug.hpp"
#include "json_utils.hpp"
#include "parse.hpp"
#include "utils.hpp"
#include "web.hpp"
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <nlohmann/json.hpp>
#include <thread>

namespace beast     = boost::beast;
namespace asio      = boost::asio;
namespace websocket = beast::websocket;
namespace j         = qb::json_utils;
using json          = nlohmann::json;

qb::Bot::Bot(const Flag flag)
{
    if (qb::bitwise_and<int>(flag, Flag::LazyInit)) return;
    start();
}

/*****
 *
 * Main bot logic - Handlers for events & major opcodes.
 *
 *****/

void qb::Bot::handle_hello(const json& payload)
{
    qb::log::point("Received Hello payload. Retrieving heartbeat interval.");
    hb_interval_ms_ = payload["d"]["heartbeat_interval"].get<unsigned int>();
    qb::log::value("heartbeat_interval", hb_interval_ms_);

    // After receiving the Hello payload, we must identify with the remote server.
    const auto identify_packet = j::get_identify_packet(qb::detail::get_bot_token());
    qb::log::data("Identification payload", identify_packet.dump(2));

    qb::log::point("Writing identification payload to websocket.");
    dispatch_write(identify_packet.dump());
}

void qb::Bot::handle_event(const json& payload)
{
    const auto et = j::def(payload, "t", std::string{"ERR"});
    if (et == "MESSAGE_CREATE")
    {
        qb::log::point("A message was created.");
        // Print it for debug for now
        qb::log::data("Message", payload.dump(2));

        // New Message!
        const auto cmd = qb::parse::remove_non_cmd(payload["d"]["content"]);
        qb::log::point("Attempting to parse command: ", cmd);
        if (cmd == "stop") shutdown();
        if (cmd == "try") send("Hello world!!!", payload["d"]["guild_id"]);
    }
    else if (et == "READY")
    {
        qb::log::point("A ready payload was sent.");
    }
}

void qb::Bot::send(std::string msg, std::string channel)
{
    json msg_json{{"content", msg}};
    const auto resp = web_ctx_->post(web::Endpoint::channels, channel, msg_json.dump()); 
    qb::log::data("Response", resp.dump(2));
}

/*****
 *
 * Core bot code - Read and write messages asynchronously.
 *
 *****/

void qb::Bot::dispatch_ping_in(unsigned int ms)
{
    if (!ws_) return;
    timer_->expires_from_now(boost::asio::chrono::milliseconds(ms));
    timer_->async_wait(std::bind(&qb::Bot::ping_sender, this, std::placeholders::_1));
}

void qb::Bot::dispatch_write(const std::string& str)
{
    assert(!outstanding_write_);
    if (write_outgoing_)
    {
        qb::log::data("Outgoing message string", str);
    }
    if (!ws_)
    {
        qb::log::point("Not dispatching write due to shutdown.");
        return;
    }
    (*ws_)->async_write(asio::buffer(str), std::bind(&qb::Bot::write_complete_handler, this,
                                                     std::placeholders::_1, std::placeholders::_2));
    outstanding_write_ = true;
}

void qb::Bot::dispatch_read()
{
    // If the websocket is closed, we're shutting down.
    if (!ws_)
    {
        qb::log::point("Not dispatching read due to shutdown.");
        return;
    }
    (*ws_)->async_read(
        buffer_, std::bind(&qb::Bot::read_handler, this, std::placeholders::_1, std::placeholders::_2));
}

void qb::Bot::ping_sender(const boost::system::error_code& error)
{
    /** This function continuously sends 'heartbeats'.
        Essentially, it will intermittently send a basic payload where
        the only important piece of data is the opcode 1. **/
    if (error)
    {
        qb::log::err(error.message());
        return;
    }
    if (hb_interval_ms_ < 11)
    {
        qb::log::warn("Pausing ping sending due to short interval of ", hb_interval_ms_,
                      " milliseconds.");
        dispatch_ping_in(10000);
        return;
    }
    // Check if we've already fired an async_write
    if (outstanding_write_)
    {
        // Pings aren't that important, try again in a second
        qb::log::point("Outstanding write, skipping ping for a second.");
        dispatch_ping_in(1000);
        return;
    }
    if (acks_received_ < pings_sent_)
    {
        qb::log::warn("Got ", acks_received_, " acks & sent ", pings_sent_, " pings. Waiting...");
        dispatch_ping_in(2000);
        return;
    }

    // Send a ping
    qb::log::point("Sending ping.");
    dispatch_write(heartbeat_msg_);
    pings_sent_ += 1;
    dispatch_ping_in(hb_interval_ms_);
}

void qb::Bot::write_complete_handler(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    // Our most recent write is now complete. That's great!
    qb::log::point("Completed a write with", bytes_transferred, "bytes transferred.");
    if (error)
    {
        qb::log::err(error.message(), '|', error.category().name(), ':', error.value());
    }
    outstanding_write_ = false; // We can write again
}

void qb::Bot::read_handler(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    qb::log::point("Parsing received data. Bytes transferred: ", bytes_transferred);
    if (error)
    {
        qb::log::err(error.message(), " (", error.category().name(), ':', error.value(), ')');
        return;
    }
    assert(bytes_transferred != 0);

    // Read the received data into a string.
    const auto resp = json::parse(beast::buffers_to_string(buffer_.data()));
    // Clear the buffer as soon as possible.
    buffer_.consume(buffer_.size());

    if (write_incoming_)
    {
        qb::log::data("Read incoming data", resp.dump(2));
    }

    // TODO - Each opcode should probably be handled in its own function.
    // Handle each opcode separately
    switch (j::def(resp, "op", -1))
    {
    case 10:
        handle_hello(resp);
        break;
    case 0:
        handle_event(resp);
        break;
    case 11:
        // Handle ACK here because it's easy
        qb::log::point("Received ACK.");
        acks_received_ += 1;
        break;
    case -1:
        qb::log::err("Could not find opcode in response: ", resp.dump());
        break;
    default:
        qb::log::warn("No handler implemented for data: ", resp.dump());
        break;
    }

    // We must always recursively continue to read more data.
    qb::log::point("Dispatching new read.");
    dispatch_read();
}

void qb::Bot::start()
{
    // Some basic initialization prior to starting any networking calls.
    qb::log::point("Creating a web context.");
    web::context web_context;
    web_context.initialize();
    web_ctx_ = &web_context;

    qb::log::point("Creating timer for ping operations.");
    timer_.emplace(*web_context.ioc_ptr(), boost::asio::chrono::milliseconds(hb_interval_ms_));

    // Make API call to Discord /gateway/bot/ to get a WebSocket URL
    auto socket_info             = web_context.get(web::Endpoint::gateway_bot);
    const std::string socket_url = socket_info["url"];
    qb::log::data("Socket information", socket_info.dump(2));

    // Acquire a websocket connection to the URL.
    ws_.emplace(std::move(web_context.acquire_websocket(socket_url)));

    // Start the asynchronous read loop.
    dispatch_read();

    // Start the asynchronous write loop.
    ping_sender({});

    // Begin allowing completion handlers to fire.
    web_context.run();

    // End bot execution.
    qb::log::point("Finishing bot execution...");
}

void qb::Bot::shutdown()
{
    qb::log::point("Beginning shutdown.");
    // Stop the timer.
    timer_->cancel();
    // Close the websocket.
    try
    {
        ws_->disconnect();
    }
    catch (const std::exception& e)
    {
        qb::log::warn("Error while attempting to disconnect websocket: ", e.what());
    }
    ws_.reset();
    qb::log::point("Shutdown completed.");
}
