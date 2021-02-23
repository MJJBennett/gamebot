#ifndef QUEUE_CLASS_HPP
#define QUEUE_CLASS_HPP

#include <boost/asio/steady_timer.hpp>
#include <memory>
#include <string>
#include <vector>

namespace qb
{
class queue
{
public:
    queue(std::string guild_id, std::string channel_id, int, boost::asio::io_context* ioc)
        : channel_id_(channel_id),
          guild_id_(guild_id),
          timer_(std::make_unique<boost::asio::steady_timer>(*ioc))
    {
    }

    queue(std::string guild_id, std::string channel_id, int)
        : channel_id_(channel_id), guild_id_(guild_id), timer_(nullptr)
    {
    }

    template <typename F, typename T>
    void async_wait(F&& f, T&& t)
    {
        assert(timer_);
        timer_->expires_after(t);
        timer_->async_wait(f);
    }

    std::string channel_id_;

    bool is_timed()
    {
        return (bool)timer_;
    }

private:
    // Wraps a queue
    std::string guild_id_;

    std::vector<std::string> choices_;

    std::unique_ptr<boost::asio::steady_timer> timer_;
};

} // namespace qb

#endif // QUEUE_CLASS_HPP
