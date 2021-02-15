#ifndef QB_LOOP_HPP
#define QB_LOOP_HPP

/**
 * ASIO loop wrapper that contains some common utilities.
 * Also helps with bot interactions.
 * TODO
 */

#include <chrono>

namespace boost::asio
{
class io_context;
}

namespace qb
{
class Bot;

struct IOLoop
{
    // This is spooky
    boost::asio::io_context* io_context = nullptr;
};
} // namespace qb

#endif // QB_LOOP_HPP
