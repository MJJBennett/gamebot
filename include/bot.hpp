#ifndef BOT_HPP
#define BOT_HPP

#include "web.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/system/error_code.hpp>
#include <optional>

namespace qb
{
class Bot
{
public:
    // Types
    enum class Flag : int
    {
        None     = 0,
        LazyInit = 1
    };

public:
    explicit Bot(Flag = Flag::None);
    void start(); // Runs the bot

private:
    // Asynchronous recursive method, continously sends heartbeats across the WebSocket connection.
    void ping_sender(const boost::system::error_code& error);
    // Asynchronous recursive method, parses most recent read data then starts another async_read.
    void read_handler(const boost::system::error_code& error, std::size_t bytes_transferred);
    // Called after any async_write completes.
    void write_complete_handler(const boost::system::error_code& error, std::size_t bytes_transferred);

    // Helper method: Sends a ping `ms` milliseconds after being called, asynchronously.
    void dispatch_ping_in(unsigned int ms);
    // Helper method: Writes a string to the WebSocket connection. Sets outstanding_write.
    void dispatch_write(const std::string& str);

private:
    std::optional<web::WSWrapper> ws_;                 // WebSocket connection
    boost::asio::io_context ioc_{};                    // IO Context handler
    boost::beast::flat_buffer buffer_;                 // Persistent read buffer
    std::optional<boost::asio::steady_timer> timer_{}; // Persistent write timer

    unsigned int hb_interval_ms_{0};      // Interval between heartbeats.
    bool outstanding_write_{false};       // True if an async_write is currently in progress.
    unsigned long long pings_sent_{0};    // Number of heartbeats that have been sent.
    unsigned long long acks_received_{0}; // Number of heartbeat ACKs that have been received.

private:
    // Heartbeat data (opcode 1)
    const std::string heartbeat_msg_{
        nlohmann::json{{"op", 1}, {"s", nullptr}, {"d", {}}, {"t", nullptr}}.dump()};
};
} // namespace qb

#endif // BOT_HPP
