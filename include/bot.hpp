#ifndef BOT_HPP
#define BOT_HPP

#include <boost/system/error_code.hpp>
#include <boost/asio/io_context.hpp>
#include "web.hpp"

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
    void ping_sender(const boost::system::error_code& error);

private:
    unsigned int hb_interval_ms_ = 0;
    web::WSWrapper ws_{};
    boost::asio::io_context ioc_{};
};
} // namespace qb

#endif // BOT_HPP
