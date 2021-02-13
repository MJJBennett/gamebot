#ifndef BOT_HPP
#define BOT_HPP

#include "components/action.hpp"
#include "components/queue.hpp"
#include "web/web.hpp"
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/system/error_code.hpp>
#include <optional>
#include <unordered_map>

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
    ~Bot();
    explicit Bot(Flag = Flag::None);
    void start();    // Runs the bot
    void shutdown(); // Stops all asynchronous operations.

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
    // Helper method: Starts an async_read on the WebSocket connection.
    void dispatch_read();

private:
    /** opcode and event handlers **/
    void handle_hello(const nlohmann::json& payload);
    void handle_event(const nlohmann::json& payload);

public: /** Send is a part of our public API currently. */
    // Sends a message in a Discord channel.
    void send(std::string msg, std::string channel);

private:
    /** Command handlers. **/
    void print(const std::string& cmd, const std::string& channel);
    void queue(const std::string& cmd, const nlohmann::json& data);
    void store(const std::string& cmd, const std::string& channel);
    void recall(const std::string& cmd, const std::string& channel);
    void list(const std::string& cmd, const std::string& channel);

    void configure(const std::string& cmd, const nlohmann::json& data);
    void assign_emote(const std::string& cmd, const std::string& channel);

private:
    void handle_queue_timeout(const std::string& message_id, const boost::system::error_code& error);

    bool is_identity(const nlohmann::json& data);

private:
    std::optional<web::WSWrapper> ws_;                 // WebSocket connection
    boost::beast::flat_buffer buffer_;                 // Persistent read buffer
    std::optional<boost::asio::steady_timer> timer_{}; // Persistent write timer
    web::context* web_ctx_{nullptr};                   // Provides HTTP operations

    std::optional<std::string> identity_;

    unsigned int hb_interval_ms_{0};      // Interval between heartbeats.
    bool outstanding_write_{false};       // True if an async_write is currently in progress.
    unsigned long long pings_sent_{0};    // Number of heartbeats that have been sent.
    unsigned long long acks_received_{0}; // Number of heartbeat ACKs that have been received.
    unsigned int failed_ack_searches_{0}; // Number of reads for ACKs that have failed in a row.

    bool mode_1984_{false}; // WIP/TODO - Sentiment analysis/positive (re)enforcement.

    bool write_incoming_{false}; // Debug - Fully print incoming WebSocket data.
    bool write_outgoing_{false}; // Debug - Fully print outgoing WebSocket data.
    bool log_loud_{false};       // Debug - For lack of a better setup, this prints more.

    std::unordered_map<std::string, qb::queue> queues_; // Our actual queues.

    // Hashmap of callbacks; these are our commands.
    ::qb::Actions actions_;

private:
    // Heartbeat data (opcode 1). Sent across WebSocket connection at regular intervals.
    const std::string heartbeat_msg_{
        nlohmann::json{{"op", 1}, {"s", nullptr}, {"d", {}}, {"t", nullptr}}.dump()};

    bool dead{true};
};

} // namespace qb

#endif // BOT_HPP
