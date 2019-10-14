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
namespace asio      = boost::asio;
namespace websocket = beast::websocket;
using json          = nlohmann::json;

qb::Bot::Bot(const Flag flag)
{
    if (qb::bitwise_and<int>(flag, Flag::LazyInit)) return;
    start();
}

void qb::Bot::dispatch_ping_in(unsigned int ms)
{
    timer_->expires_from_now(boost::asio::chrono::milliseconds(ms));
    timer_->async_wait(std::bind(&qb::Bot::ping_sender, this, std::placeholders::_1));
}

void qb::Bot::dispatch_write(const std::string& str)
{
    assert(!outstanding_write_);
    (*ws_)->async_write(asio::buffer(str), std::bind(&qb::Bot::write_complete_handler, this,
                                                     std::placeholders::_1, std::placeholders::_2));
    outstanding_write_ = true;
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

    // TODO - Each opcode should probably be handled in its own function.
    if (qb::json_utils::val_eq(resp, "op", 10))
    {
        qb::log::point("Received Hello payload. Retrieving heartbeat interval.");
        hb_interval_ms_ = resp["d"]["heartbeat_interval"].get<unsigned int>();
        qb::log::value("heartbeat_interval", hb_interval_ms_);

        // After receiving the Hello payload, we must identify with the remote server.
        const auto identify_packet = qb::json_utils::get_identify_packet(qb::detail::get_bot_token());
        qb::log::data("Identification payload", identify_packet.dump(2));

        qb::log::point("Writing identification payload to websocket.");
        if (outstanding_write_)
        {
            qb::log::err("Write in progress while attempting to identify with remote server.");
            return;
        }
        outstanding_write_ = true;
        dispatch_write(identify_packet.dump());
    }
    else if (qb::json_utils::val_eq(resp, "op", 0))
    {
        qb::log::point("Successfuly received OPCODE 0 payload...");
    }
    else if (qb::json_utils::val_eq(resp, "op", 11))
    {
        qb::log::point("Received ACK.");
        acks_received_ += 1;
    }
    else
    {
        qb::log::data("Payload read", resp.dump(2));
    }

    // We must always recursively continue to read more data.
    qb::log::point("Read started...");
    (*ws_)->async_read(
        buffer_, std::bind(&qb::Bot::read_handler, this, std::placeholders::_1, std::placeholders::_2));
}

void qb::Bot::start()
{
    // Some basic initialization prior to starting any networking calls.
    qb::log::point("Creating timer for ping operations.");
    timer_.emplace(ioc_, boost::asio::chrono::milliseconds(hb_interval_ms_));

    // Make API call to Discord /gateway/bot/ to get a WebSocket URL
    auto socket_info             = web::get_bot_socket(ioc_);
    const std::string socket_url = socket_info["url"];
    qb::log::data("Socket information", socket_info.dump(2));

    // Acquire a websocket connection to the URL.
    ws_.emplace(std::move(web::acquire_websocket(socket_url, ioc_)));

    // Start the asynchronous read loop.
    (*ws_)->async_read(
        buffer_, std::bind(&qb::Bot::read_handler, this, std::placeholders::_1, std::placeholders::_2));

    // Start the asynchronous write loop.
    ping_sender({});

    // Begin allowing completion handlers to fire.
    ioc_.run();

    // End bot execution.
    qb::log::point("Finishing bot execution...");
}
