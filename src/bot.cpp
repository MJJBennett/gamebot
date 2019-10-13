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

void qb::Bot::ping_sender(const boost::system::error_code& error)
{
    if (error)
    {
        qb::log::err(error.message());
        return;
    }
    if (hb_interval_ms_ < 11)
    {
        qb::log::warn("Very short heartbeat interval of", hb_interval_ms_,
                      "- pausing for a few seconds.");

        timer_->expires_from_now(boost::asio::chrono::seconds(5));
        timer_->async_wait(std::bind(&qb::Bot::ping_sender, this, std::placeholders::_1));
    }
    // Check if we've already fired an async_write
    if (outstanding_write_)
    {
        // Pings aren't that important, try again in a second
        qb::log::point("Outstanding write, skipping ping for a second.");
        timer_->expires_from_now(boost::asio::chrono::milliseconds(hb_interval_ms_));
        timer_->async_wait(std::bind(&qb::Bot::ping_sender, this, std::placeholders::_1));
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
    timer_->expires_from_now(boost::asio::chrono::milliseconds(hb_interval_ms_));
    timer_->async_wait(std::bind(&qb::Bot::ping_sender, this, std::placeholders::_1));
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
        if (error != asio::error::eof && error != ssl::error::stream_truncated)
        {
            qb::log::warn("Ending read loop due to above error.");
            return;
        }
    }
    const auto read_data = beast::buffers_to_string(buffer_.data());
    qb::log::value("Bytes transferred", bytes_transferred);
    if (bytes_transferred > 1)
    {
        // Handle Hello packet

        const auto resp = json::parse(beast::buffers_to_string(buffer_.data()));
        // Buffer is cleared below
        if (qb::json_utils::val_eq(resp, "op", 10))
        {
            qb::log::point("Received hello package. Bytes Transferred:", bytes_transferred);
            qb::log::point("Retrieving heartbeat interval.");
            hb_interval_ms_ = resp["d"]["heartbeat_interval"].get<unsigned int>();
            qb::log::value("heartbeat_interval", hb_interval_ms_);
            // Step 12 - Generate the identification packet. (Also Step 13, Step 14 - Adding token/properties)
            const auto identify_packet = qb::json_utils::get_identify_packet(qb::detail::get_bot_token());
            qb::log::data("Identification payload", identify_packet.dump(2));

            qb::log::point("Writing identification payload to websocket.");
            if (outstanding_write_)
            {
                qb::log::point("Okay... there's a write in progress. What?");
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
            qb::log::point("Received some payload. OPCODE:", resp["op"]);
        }

        // qb::log::data("Data read from websocket", qb::json_utils::try_prettify(read_data));
        // qb::log::data("RAW DATA", read_data);
    }
    else
    {
        // What's going on..?
        misc_counter_++;
        if (misc_counter_ % 10 == 0 && misc_counter_ < 100)
        {
            qb::log::point("This point you know what it means. Bytes transferred:", bytes_transferred);
            qb::log::value("Misc counter", misc_counter_);
        }
    }
    buffer_.consume(buffer_.size());
    (*ws_)->async_read(
        buffer_, std::bind(&qb::Bot::read_handler, this, std::placeholders::_1, std::placeholders::_2));
}

void qb::Bot::start()
{
    qb::log::point("Creating timer for ping operations.");
    timer_.emplace(ioc_, boost::asio::chrono::milliseconds(hb_interval_ms_));
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
    ws_.emplace(std::move(web::acquire_websocket(socket_url, ioc_)));
    auto& ws = *ws_;

    /** Step 08 - Receive OPCODE 10 packet. **/
    qb::log::point("Reading the Hello payload into the buffer.");
    ws->async_read(
        buffer_, std::bind(&qb::Bot::read_handler, this, std::placeholders::_1, std::placeholders::_2));

    ping_sender({});
    ioc_.run();

    // using namespace std::chrono_literals;
    // std::this_thread::sleep_for(5s);

    // For now, just disconnect again, as this is a work in progress.
    qb::log::point("Finishing bot execution...");
}
