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

const boost::asio::const_buffers_1 qb::Bot::heartbeat_msg_ =
    boost::asio::buffer(nlohmann::json{{"op", 1}, {"s", nullptr}, {"d", {}}, {"t", nullptr}}.dump());

qb::Bot::Bot(const Flag flag)
{
    using namespace qb;
    if (bitwise_and<int>(flag, Flag::LazyInit)) return;
    start();
}

void qb::Bot::dispatch_ping_in(unsigned int ms)
{
    timer_->expires_from_now(boost::asio::chrono::milliseconds(ms));
    timer_->async_wait(std::bind(&qb::Bot::ping_sender, this, std::placeholders::_1));
}

void qb::Bot::ping_sender(const boost::system::error_code& error)
{
    if (error)
    {
        qb::log::err(error.message());
        return;
    }
    if (hb_interval_ms_ < 11)
    {
        qb::log::warn("Pausing ping sending due to short interval:", hb_interval_ms_);
        dispatch_ping_in(5000);
        return;
    }
    // Check if we've already fired an async_write
    if (outstanding_write_)
    {
        // Pings aren't that important, try again in a second
        qb::log::point("Outstanding write, skipping ping for a second.");
        dispatch_ping_in(10000);
        return;
    }
#ifdef CAREFUL_NO_DDOS
    if (pings_sent_ > 10)
    {
        qb::log::warn(
            "Let's not DDOS anyone! Stopping sending pings. Service will shut down in ~40-120s. "
            ":(");
        return;
    }
#endif
    // Send a ping
    qb::log::point("Sending ping.");
    (*ws_)->async_write(heartbeat_msg_, std::bind(&qb::Bot::write_complete_handler, this,
                                                  std::placeholders::_1, std::placeholders::_2));
    pings_sent_ += 1;
    outstanding_write_ = true;
    dispatch_ping_in(hb_interval_ms_);
}

void qb::Bot::write_complete_handler(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    // Our most recent write is now complete. That's great!
    qb::log::point("Completed write,", bytes_transferred, "bytes transferred.");
    if (error)
    {
        qb::log::err(error.message(), '|', error.category().name(), ':', error.value());
    }
    outstanding_write_ = false;
}

void qb::Bot::read_handler(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if (error)
    {
        qb::log::err(error.message(), '|', error.category().name(), ':', error.value(),
                     "| Bytes Transferred:", bytes_transferred);
        return;
    }
    assert(bytes_transferred != 0);

    // Print diagnostics.
    qb::log::point("[READ] Bytes Transferred:", bytes_transferred);

    // Read the received data into a string.
    const auto read_data = beast::buffers_to_string(buffer_.data());
    // Clear the buffer as soon as possible.
    buffer_.consume(buffer_.size());
    // Now generate a proper JSON response object.
    const auto resp = json::parse(beast::buffers_to_string(buffer_.data()));

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
        (*ws_)->async_write(asio::buffer(identify_packet.dump()),
                            std::bind(&qb::Bot::write_complete_handler, this,
                                      std::placeholders::_1, std::placeholders::_2));
    }
    else if (qb::json_utils::val_eq(resp, "op", 0))
    {
        qb::log::point("Successfuly received OPCODE 0 payload...");
    }
    else
    {
        qb::log::data("Payload read", resp.dump(2));
    }

    // We must always recursively continue to read more data.
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
