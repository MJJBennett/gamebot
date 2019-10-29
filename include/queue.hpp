#ifndef QUEUE_CLASS_HPP
#define QUEUE_CLASS_HPP

#include <boost/asio/steady_timer.hpp>
#include <optional>
#include <string>
#include <vector>

class queue
{
public:
    queue(std::string guild_id, std::string channel_id)
        : guild_id_(guild_id), channel_id_(channel_id)
    {
    }

private:
    // Wraps a queue
    std::string guild_id_;
    std::string channel_id_;

    std::vector<std::string> choices_;

    std::optional<boost::asio::steady_timer> timer_;
};

#endif // QUEUE_CLASS_HPP
