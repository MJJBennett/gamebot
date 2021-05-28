#ifndef ASYNC_STDIN_HPP
#define ASYNC_STDIN_HPP

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#define QB_USING_STDIN 1
// #pragma message "Compiling with STDIN support ENABLED."

#include "debug.hpp"

#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <optional>
#include <stdio.h>

namespace qb
{
struct stdin_io
{
    static constexpr size_t MAX_SIZE = 256;

    template <typename F>
    stdin_io(boost::asio::io_context& ioc, F&& handler)
        : io_stream(ioc, ::dup(STDIN_FILENO) /* Duplicates the stream */), io_handler(handler)
    {
        qb::log::point("Created stdin i/o.");
    }

    std::string read(std::size_t bytes)
    {
        // maybe replace with this later
        // https://stackoverflow.com/questions/877652/copy-a-streambufs-contents-to-a-string
        if (bytes == 0) return {};
        std::string out;
        out.resize(bytes - 1);
        io_buffer.sgetn(out.data(), bytes - 1);
        io_buffer.consume(1);
        return out;
    }

    void cleanse()
    {
        io_buffer.consume(io_buffer.size());
    }

    void async_read()
    {
        qb::log::point("Launching stdin i/o.");
        boost::asio::async_read_until(io_stream, io_buffer, '\n', io_handler);
    }

    void close()
    {
        qb::log::point("Shutting down stdin i/o.");
        io_stream.close();
    }

    boost::asio::posix::stream_descriptor io_stream;
    boost::asio::streambuf io_buffer{MAX_SIZE};
    std::function<void(const boost::system::error_code& error, std::size_t bytes_transferred)> io_handler;
};
} // namespace qb
#else
namespace qb
{
struct stdin_io
{
    template <typename F>
    stdin_io(boost::asio::io_context&, F&&)
    {
    }

    void cleanse()
    {
    }

    void async_read()
    {
    }

    void close()
    {
    }

    std::string read(std::size_t bytes)
    {
        return "";
    }
};
} // namespace qb
#endif

#endif // ASYNC_STDIN_HPP
