#ifndef BOT_HPP
#define BOT_HPP

#include "web.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/system/error_code.hpp>
#include <optional>

#define CAREFUL_NO_DDOS 1

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
    void start();

private:
    void async_write(const boost::asio::mutable_buffer& to_write);

    void ping_sender(const boost::system::error_code& error);
    void read_handler(const boost::system::error_code& error, std::size_t bytes_transferred);
    void write_complete_handler(const boost::system::error_code& error, std::size_t bytes_transferred);

    void dispatch_ping_in(unsigned int ms);

private:
    unsigned int hb_interval_ms_{0};
    std::optional<web::WSWrapper> ws_;
    boost::asio::io_context ioc_{};
    // Check this before running an async write
    bool outstanding_write_{false};
    boost::beast::flat_buffer buffer_;
    std::optional<boost::asio::steady_timer> timer_{};

    const static boost::asio::const_buffers_1 heartbeat_msg_;
    
    unsigned int misc_counter_{0};
    unsigned long long pings_sent_{0};
};
} // namespace qb

#endif // BOT_HPP
