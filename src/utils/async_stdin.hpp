#ifndef ASYNC_STDIN_HPP
#define ASYNC_STDIN_HPP

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <boost/asio/buffer.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/asio.hpp>
#include <optional>
#include <stdio.h>
static std::optional<boost::asio::posix::stream_descriptor> io_stream;
static boost::beast::flat_buffer io_buffer;
namespace qb
{
template <typename ioc_t, typename F>
void async_stdin_read(ioc_t& ioc, F&& handler)
{
    using namespace boost::asio;
    if (!io_stream)
    {
        io_stream = posix::stream_descriptor(ioc, STDIN_FILENO);
    }
    async_read(*io_stream, io_buffer, handler);
}
} // namespace qb
#else
template <typename ioc_t, typename F>
void async_stdin_read(ioc_t& ioc, F&& handler)
{
    return;
}
#endif

#endif // ASYNC_STDIN_HPP
