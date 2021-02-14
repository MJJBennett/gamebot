#include "bot.hpp"

#include "components/messages.hpp"
#include "components/sentiment.hpp"
#include "components/games/hangman.hpp"
#include "utils/debug.hpp"
#include "utils/fileio.hpp"
#include "utils/json_utils.hpp"
#include "utils/parse.hpp" // For command parsing
#include "utils/utils.hpp" // For get_bot_token
#include <algorithm>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

namespace beast     = boost::beast;
namespace asio      = boost::asio;
namespace websocket = beast::websocket;
namespace j         = qb::json_utils;
using json          = nlohmann::json;

qb::Bot::Bot(const Flag flag)
{
    if (qb::bitwise_and<int>(flag, Flag::LazyInit)) return;
    start();
}

/*****
 *
 * Main bot logic - Handlers for events & major opcodes.
 *
 *****/

void qb::Bot::handle_hello(const json& payload)
{
    qb::log::point("Received Hello payload. Retrieving heartbeat interval.");
    hb_interval_ms_ = payload["d"]["heartbeat_interval"].get<unsigned int>();
    qb::log::value("heartbeat_interval", hb_interval_ms_);

    // After receiving the Hello payload, we must identify with the remote server.
    const auto identify_packet = j::get_identify_packet(qb::detail::get_bot_token());
    // qb::log::data("Identification payload", identify_packet.dump(2));

    qb::log::point("Writing identification payload to websocket.");
    dispatch_write(identify_packet.dump());
}

/** Event handling - all user-interaction code starts here. **/
void qb::Bot::handle_event(const json& payload)
{
    using namespace qb::parse;
    namespace messages = qb::messages;

    const auto et = j::def(payload, "t", std::string{"ERR"});
    if (et == "MESSAGE_CREATE")
    {
        if (log_loud_) qb::log::point("A message was created.");
        // qb::log::data("Message", payload.dump(2));

        if (is_identity(payload))
        {
            qb::log::point("Ignoring self message create.");
            return;
        }

        // New Message!
        const auto contents = payload["d"]["content"];
        if (qb::parse::is_command(contents))
        {
            const auto cmd     = qb::parse::get_command(contents);
            const auto channel = payload["d"]["channel_id"];
            qb::log::point("Attempting to parse command: ", cmd);

            if (startswithword(cmd, "stop"))
                shutdown();
            else if (startswith(cmd, "print "))
                print(cmd, channel);
            else if (startswith(cmd, "queue "))
                queue(cmd, payload["d"]);
            else if (startswith(cmd, "s"))
                store(cmd, channel);
            else if (startswithword(cmd, "online"))
                send(qb::messages::online(), channel);
            else if (startswithword(cmd, "list"))
                list(cmd, channel);
            else if (startswithword(cmd, "help"))
                send(qb::messages::help, channel);
            else if (startswithword(cmd, "db:writeincoming"))
                write_incoming_ = true;
            else if (startswithword(cmd, "db:nowriteincoming"))
                write_incoming_ = false;
            else if (startswithword(cmd, "db:loud"))
                log_loud_ = true;
            else if (startswithword(cmd, "db:noloud"))
                log_loud_ = false;
            else if (startswithword(cmd, "db:verboseweb"))
                web_ctx_->debug_mode(true);
            else if (startswithword(cmd, "db:noverboseweb"))
                web_ctx_->debug_mode(false);
            else if (startswithword(cmd, "recall"))
                recall(cmd, channel);
            else if (startswithword(cmd, "db:get_emotes"))
                recall_emote(cmd, channel);
            else if (startswith(cmd, "conf"))
                configure(cmd, payload["d"]);
            else if (startswithword(cmd, "assign"))
                assign_emote(cmd, channel);
            else
            {
                // Check if we have an action bound for this command
                if (const auto& a = qb::parse::get_command_name(cmd); actions_.find(a) != actions_.end())
                {
                    qb::log::point("Found an action based command.");
                    const auto _unused_retval = actions_[a](cmd, api::Message::create(payload["d"]), *this);
                }
            }
        }
        else if (mode_1984_)
        {
            if (qb::sentiment::is_negative(contents))
            {
                send("Warning: The message `" + std::string{contents} +
                         "` is overly negative. You may be required to report for psychological "
                         "evaluation at later date.",
                     payload["d"]["channel_id"]);
            }
        }
    }
    else if (et == "INTERACTION_CREATE")
    {
        qb::log::point("An interaction was sent.");
        const auto d = payload["d"];
        if (const auto interact_id = j::get_opt<std::string>(d, "id"); interact_id)
        {
            if (const auto interact_tok = j::get_opt<std::string>(d, "token"); interact_tok)
            {
                json interact_json{{"type", 4}, {"data", {{"content", "Got your command!"}}}};
                const auto _ = web_ctx_->post(
                    web::Endpoint::interactions,
                    std::vector<std::string>{*interact_id, *interact_tok, "callback"},
                    interact_json.dump());
            }
        }
    }
    else if (et == "READY")
    {
        qb::log::point("A ready payload was sent.");
    }
}

void qb::Bot::send(std::string msg, std::string channel)
{
    if (msg.size() > 2000)
    {
        send("I can't do that. [Message length too large: " + std::to_string(msg.size()) +
                 " - Must be 2000 or less.]",
             channel);
        return;
    }
    json msg_json{{"content", msg}};
    const auto resp = web_ctx_->post(web::Endpoint::channels, channel, msg_json.dump());
    if (!identity_)
    {
        identity_ = resp["author"]["id"];
        qb::log::point("Setting identity to: ", *identity_);
    }
    if (log_loud_) qb::log::data("Response", resp.dump(2));
}

bool qb::Bot::is_identity(const nlohmann::json& msg)
{
    if (!identity_) return false;
    try
    {
        return msg["d"]["author"]["id"] == *identity_;
    }
    catch (const std::exception& e)
    {
        qb::log::err("Caught some error ", e.what());
    }
    return false;
}

/*****
 *
 * User interaction logic.
 *
 ****/

void qb::Bot::print(const std::string& cmd, const std::string& channel)
{
    send(cmd.substr(6), channel);
}

void qb::Bot::list(const std::string& cmd, const std::string& channel)
{
    // We need to build a list of all the JSON keys
    send(qb::messages::keys(qb::parse::concatenate(qb::fileio::skribbl::keys())), channel);
}

void qb::Bot::queue(const std::string& cmd, const nlohmann::json& data)
{
    const std::string channel = data["channel_id"];
    const std::string guild   = data["guild_id"];
    const std::string msg_id  = data["id"];

    using namespace qb::parse;
    auto contents = split(cmd.substr(6));

    /*
    // Should be in the format of [time] [choices...]
    // Order should be irrelevant, assuming time is properly formatted.
    auto [time, games] = get_time(contents);

    if (time == "")
    {
    //    send(qb::messages::queue_needs_time(), channel);
    }*/

    // Functionality before abstraction
    // !qb queue Activity [Max Participants | Timeout]

    if (contents.size() != 2)
    {
        send(
            "Current format for queue is: queue [Activity] [Time | Max Participants] (e.g. queue "
            "Soccer 3m or queue Chess 2)",
            channel);
        return;
    }

    int param = 0;
    bool time = false;
    try
    {
        size_t p{0};
        param = stoi(contents.at(1), &p);

        if (p != contents.at(1).size())
        {
            time = true;
        }
    }
    catch (const std::invalid_argument&)
    {
        send(
            "Current format for queue is: queue [Activity] [Time | Max Participants] (e.g. queue "
            "Soccer 3m or queue Chess 2)",
            channel);
        return;
    }

    if (time)
        qb::log::point("Starting time queue with value of ", param);
    else
        qb::log::point("Starting person queue with value of ", param);

    if (time)
    {
        auto [it, b] = queues_.emplace(std::piecewise_construct, std::make_tuple(msg_id),
                                       std::make_tuple(guild, channel, param, web_ctx_->ioc_ptr()));
        if (!b)
        {
            send("Something failed when creating the queue. Please try again!", channel);
        }
        it->second.async_wait(std::bind(&qb::Bot::handle_queue_timeout, this, msg_id, std::placeholders::_1),
                              std::chrono::minutes(param));
    }
    else
    {
        auto [it, b] = queues_.emplace(std::piecewise_construct, std::make_tuple(msg_id),
                                       std::make_tuple(guild, channel, param));
        if (!b)
        {
            send("Something failed when creating the queue. Please try again!", channel);
        }
    }

    /*
    // Write our own parsing logic here, for now
    // It can be assumed that data is valid and contains what it must
    if (queues_.find(guild) == queues_.end())
    {
        queues_.emplace(guild, std::vector<nlohmann::json>{});
    }*/

    // send(messages::queue_start(contents), channel);
    send("Queuing for " + contents.at(0) + "! Respond to this message with " +
             qb::fileio::get_emote("yes") + " to join the queue, " +
             qb::fileio::get_emote("maybe") + " to indicate a 'maybe', or " +
             qb::fileio::get_emote("no") + " to indicate a definite no.",
         channel);
}

void qb::Bot::handle_queue_timeout(const std::string& message_id, const boost::system::error_code& error)
{
    qb::log::point("Handling queue timeout for message id ", message_id);
    if (error)
    {
        qb::log::err(error.message());
        return;
    }

    if (auto it = queues_.find(message_id); it != queues_.end())
    {
        std::string channel = std::move(it->second.channel_id_);
        queues_.erase(it);
        send("Queue is complete!", channel);
    }
    else
    {
        qb::log::warn("Could not find that message ID in the map.");
    }
}

void qb::Bot::store(const std::string& cmd, const std::string& channel)
{
    // This command has variants, so we need to get the actual length of the command
    auto before   = std::string{cmd.cbegin(), std::find_if(cmd.cbegin(), cmd.cend(), isspace)};
    auto contents = qb::parse::split(cmd.substr(before.size()), ',');

    std::vector<std::string> ignored;
    std::vector<std::string> stored;
    for (auto&& str : contents)
    {
        // We need to validate each string
        const auto trimmed = qb::parse::trim(str);
        if (trimmed.size() > qb::config::max_skribbl_wordsize)
        {
            ignored.emplace_back(std::move(trimmed));
            continue;
        }
        stored.emplace_back(std::move(trimmed));
    }

    // We ignored some of them, let's let the user know.
    if (!ignored.empty()) send(messages::cannot_store(ignored), channel);
    if (stored.empty()) return;
    // We need to find out if the user wants to store a specific group.
    if (auto it = std::find_if(before.begin(), before.end(), [](char c) { return c == ':'; });
        it != before.end())
    {
        if (it + 1 == before.end())
        {
            send("A group name must be specified after the colon. (e.g. recall:groupname)", channel);
            return;
        }
        std::string key{it + 1, before.end()};
        qb::fileio::add_to_set(key, stored);
        send(messages::did_store(stored, key), channel);
    }
    else
    {
        qb::fileio::add_default(stored);
        send(messages::did_store(stored), channel);
    }
}

void qb::Bot::recall(const std::string& cmd, const std::string& channel)
{
    // auto cmd_name   = qb::parse::get_command_name(cmd);
    auto components = qb::parse::split(cmd, ' ');
    if (components.size() <= 1 || components[1] == "all")
    {
        qb::log::point("Sending all in recall command.");
        send(qb::parse::concatenate(qb::fileio::get_all(), ","), channel);
    }
    else
    {
        components.erase(components.begin());
        qb::log::point("Sending ", qb::parse::concatenate(components), " in recall command.");
        if (components.size() == 1)
            send(qb::parse::concatenate(qb::fileio::get_set(components.at(0)), ","), channel);
        else
            send(qb::parse::concatenate(qb::fileio::get_sets(components), ","), channel);
    }
}

void qb::Bot::configure(const std::string& cmd, const nlohmann::json& data)
{
    using namespace qb::json_utils;
    const std::string channel = def(data, "channel_id", std::string{});
    const std::string guild   = def(data, "guild_id", std::string{});

    if (cmd.size() == 4) return;
    const auto command = cmd.substr(5);

    if (command == "1984")
    {
        mode_1984_ = true;
        send("Enabling 1985 mode.", channel);
    }
}

void qb::Bot::recall_emote(const std::string& cmd, const std::string& channel)
{
    using namespace qb::fileio;

    auto components = qb::parse::split(cmd, ' ');
    components.erase(components.begin());
    qb::log::point("Sending some emotes in recall_emote command.");
    send(qb::parse::concatenate(qb::fileio::get_emotes(components), ", "), channel);
}

void qb::Bot::assign_emote(const std::string& cmd, const std::string& channel)
{
    using namespace qb::fileio;

    auto parsed = qb::parse::split(cmd);
    if (parsed.size() != 3)
    {
        qb::log::point("-> Incorrect number of arguments.");
        send("Arguments to assign must be a single word followed by an emote.", channel);
        return;
    }
    qb::log::point("-> Registering emote.");
    register_emote(parsed.at(1), parsed.at(2));
    qb::log::point("-> Registered emote.");
    send("Success! Name " + parsed.at(1) + " registered to emote " + get_emote(parsed.at(1)) + " !", channel);
}

/*****
 *
 * Core bot code - Read and write messages asynchronously.
 *
 *****/

void qb::Bot::dispatch_ping_in(unsigned int ms)
{
    if (!ws_) return;
    timer_->expires_from_now(boost::asio::chrono::milliseconds(ms));
    timer_->async_wait(std::bind(&qb::Bot::ping_sender, this, std::placeholders::_1));
}

void qb::Bot::dispatch_write(const std::string& str)
{
    assert(!outstanding_write_);
    if (write_outgoing_)
    {
        qb::log::data("Outgoing message string", str);
    }
    if (!ws_)
    {
        qb::log::point("Not dispatching write due to shutdown.");
        return;
    }
    (*ws_)->async_write(asio::buffer(str), std::bind(&qb::Bot::write_complete_handler, this,
                                                     std::placeholders::_1, std::placeholders::_2));
    outstanding_write_ = true;
}

void qb::Bot::dispatch_read()
{
    // If the websocket is closed, we're shutting down.
    if (!ws_)
    {
        qb::log::point("Not dispatching read due to shutdown.");
        return;
    }
    (*ws_)->async_read(
        buffer_, std::bind(&qb::Bot::read_handler, this, std::placeholders::_1, std::placeholders::_2));
}

void qb::Bot::ping_sender(const boost::system::error_code& error)
{
    /** This function continuously sends 'heartbeats'.
        Essentially, it will intermittently send a basic payload where
        the only important piece of data is the opcode 1. **/
    if (error)
    {
        qb::log::err(error.message());
        return;
    }
    if (hb_interval_ms_ < 11)
    {
        qb::log::warn("Pausing ping sending due to short interval of ", hb_interval_ms_,
                      " milliseconds.");
        dispatch_ping_in(10000);
        return;
    }
    // Check if we've already fired an async_write
    if (outstanding_write_)
    {
        // Pings aren't that important, try again in a second
        qb::log::point("Outstanding write, skipping ping for a second.");
        dispatch_ping_in(1000);
        return;
    }
    if (acks_received_ < pings_sent_)
    {
        qb::log::warn("Got ", acks_received_, " acks & sent ", pings_sent_, " pings. Waiting...");
        if (failed_ack_searches_++ > 25)
        {
            shutdown();
            return;
        }
        dispatch_ping_in(2000);
        return;
    }

    // Send a ping
    if (log_loud_) qb::log::point("Sending ping.");
    dispatch_write(heartbeat_msg_);
    pings_sent_ += 1;
    dispatch_ping_in(hb_interval_ms_);
}

void qb::Bot::write_complete_handler(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    // Our most recent write is now complete. That's great!
    if (log_loud_)
        qb::log::point("Completed a write with ", bytes_transferred, " bytes transferred.");
    if (error)
    {
        qb::log::err(error.message(), '|', error.category().name(), ':', error.value());
    }
    outstanding_write_ = false; // We can write again
}

void qb::Bot::read_handler(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if (log_loud_) qb::log::point("Parsing received data. Bytes transferred: ", bytes_transferred);
    if (error)
    {
        qb::log::err(error.message(), " (", error.category().name(), ':', error.value(), ')');
        return;
    }
    assert(bytes_transferred != 0);

    // Read the received data into a string.
    const auto resp = json::parse(beast::buffers_to_string(buffer_.data()));
    // Clear the buffer as soon as possible.
    buffer_.consume(buffer_.size());

    if (write_incoming_)
    {
        qb::log::data("Read incoming data", resp.dump(2));
    }

    // TODO - Each opcode should probably be handled in its own function.
    // Handle each opcode separately
    switch (j::def(resp, "op", -1))
    {
    case 10:
        handle_hello(resp);
        break;
    case 0:
        handle_event(resp);
        break;
    case 7:
        qb::log::err("Received opcode 7: Reconnect. Not implemented! Fix this.");
        break;
    case 11:
        // Handle ACK here because it's easy
        if (log_loud_) qb::log::point("Received ACK.");
        acks_received_ += 1;
        failed_ack_searches_ = 0;
        break;
    case -1:
        qb::log::err("Could not find opcode in response: ", resp.dump());
        break;
    default:
        qb::log::warn("No handler implemented for data: ", resp.dump());
        break;
    }

    // We must always recursively continue to read more data.
    if (log_loud_) qb::log::point("Dispatching new read.");
    dispatch_read();
}

void qb::Bot::start()
{
    // Some basic initialization prior to starting any networking calls.
    qb::log::point("Creating a web context.");
    web::context web_context;
    web_context.initialize();
    web_ctx_ = &web_context;
    dead     = false;

    qb::log::point("Creating timer for ping operations.");
    timer_.emplace(*web_context.ioc_ptr(), boost::asio::chrono::milliseconds(hb_interval_ms_));

    // Make API call to Discord /gateway/bot/ to get a WebSocket URL
    auto socket_info             = web_context.get(web::Endpoint::gateway_bot);
    const std::string socket_url = socket_info["url"];
    qb::log::data("Socket information", socket_info.dump(2));

    // Acquire a websocket connection to the URL.
    ws_.emplace(std::move(web_context.acquire_websocket(socket_url)));

    // Start the asynchronous read loop.
    dispatch_read();

    // Start the asynchronous write loop.
    ping_sender({});

    /**
     * Any delayed startup should go here.
     */
    qb::Hangman hangman;
    hangman.register_actions(actions_);

    /**
     * Begin allowing completion handlers to fire.
     * Blocking call - anything after this is only executed after
     * the application stops (i.e. the bot shuts down)
     */
    web_context.run();

    // End bot execution.
    qb::log::point("Finishing bot execution...");
}

qb::Bot::~Bot()
{
    shutdown();
}

void qb::Bot::shutdown()
{
    if (dead) return;
    dead = true;
    qb::log::point("Beginning shutdown.");
    // Stop the timer.
    timer_->cancel();
    // Close the websocket.
    try
    {
        ws_->disconnect();
    }
    catch (const std::exception& e)
    {
        qb::log::warn("Error while attempting to disconnect websocket: ", e.what());
    }
    ws_.reset();
    qb::log::point("Shutdown completed.");
}


