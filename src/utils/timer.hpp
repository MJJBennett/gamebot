#ifndef QB_TIMER_HPP
#define QB_TIMER_HPP

/**
 * This class provides an easy-to-use wrapper
 * for an asynchronous callback timer.
 */

#include <memory>

// Forward declaration.
// We hide boost includes wherever possible.
namespace boost::asio
{
class steady_timer;
}

namespace qb
{
class Bot;
class Timer
{
    using timer_type = boost::asio::steady_timer;

public:
    Timer(Bot& bot);

private:
    std::unique_ptr<timer_type> timer_;
    Bot& bot_;
};
} // namespace qb

#endif // QB_TIMER_HPP
