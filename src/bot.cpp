#include "bot.hpp"

#include "components/games/gamequeue.hpp"
#include "components/games/hangman.hpp"
#include "components/messages.hpp"
#include "components/sentiment.hpp"
#include "components/tools/normal_commands.hpp"
#include "components/tools/sprint.hpp"
#include "components/tools/todo.hpp"
#include "utils/async_stdin.hpp"
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

bool qb::Bot::handle_command(const std::string& cmd, const qb::api::Message& msg, bool is_explicit)
{
    using namespace qb::parse;
    namespace messages = qb::messages;

    const auto& channel = msg.channel.id;
    qb::log::point("Attempting to parse command: ", cmd);

    if (!is_explicit) return false; // Handle as a message

    if (startswithword(cmd, "stop"))
        shutdown();
    else if (startswith(cmd, "print "))
        print(cmd, channel);
    else if (startswith(cmd, "test "))
        test(cmd.substr(5), channel);
    else if (startswith(cmd, "store"))
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
        configure(cmd, msg.channel);
    else if (startswithword(cmd, "assign"))
        assign_emote(cmd, channel);
    else if (startswithword(cmd, "db:cursed-reconnect-code-copy-test"))
    {
        qb::log::err("Simulating opcode 7.");
        qb::log::warn("Received opcode 7: Reconnect. Attempting reconnection:");
        // DO NOT start a read, we'll start one after exiting here
        attempt_ws_reconnect(false);
        qb::log::point("Finished reconnection (Cause: Simulated 7).");
    }
    else
    {
        // Check if we have an action bound for this command
        if (const auto& a = qb::parse::get_command_name(cmd); actions_.find(a) != actions_.end())
        {
            qb::log::point("Found an action based command.");
            const auto _unused_retval = actions_[a](cmd, msg, *this);
        }
        else
        {
            return false;
        }
    }
    return true;
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

        // We make callbacks right away.
        if (j::has_path(payload, "d", "id"))
            execute_callbacks(*this, payload["d"]["id"], payload["d"], message_id_callbacks_);

        if (is_identity(payload))
        {
            qb::log::point("Ignoring self message create.");
            return;
        }

        // New Message!
        const auto contents   = payload["d"]["content"];
        const auto msg        = api::Message::create(payload["d"]);
        bool isc              = qb::parse::is_command(contents);
        const std::string cmd = isc ? qb::parse::get_command(contents) : std::string{contents};
        if (handle_command(cmd, msg, isc))
        {
            return;
        }

        // NOT A COMMAND

        if (mode_1984_)
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
        qb::log::point("An interaction was sent. Executing callbacks.");
        const auto d = api::Interaction::create(payload["d"]);
        execute_callbacks<qb::api::Interaction>(*this, d.key(), d, message_interaction_callbacks_);

        /*
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
        */
    }
    else if (et == "READY")
    {
        qb::log::point("A ready payload was sent.");
        // Quick, autoload things.
        const auto autoexec = qb::fileio::readlines_nonempty(qb::config::autoexec_file());
        for (const auto& exec : autoexec)
        {
            handle_io_read_str(exec);
        }
    }
    else if (et == "MESSAGE_REACTION_ADD" || et == "MESSAGE_REACTION_REMOVE")
    {
        qb::log::point("A reaction was added to a message.");
        // Message that was reacted to
        auto msg = api::Reaction::create(payload["d"]);
        // TODO should really just be passing the message...
        qb::log::point("Execute relevant callbacks.");

        execute_callbacks<qb::api::Reaction>(
            *this, msg.message_id, msg, message_reaction_callbacks_, et == "MESSAGE_REACTION_ADD");
    }
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

bool qb::Bot::is_identity(const api::Message& message)
{
    if (!identity_) return false;
    return message.user.id == *identity_;
}

nlohmann::json qb::Bot::send_test(std::string msg, std::string channel)
{
    if (msg.size() > 2000)
    {
        send("I can't do that. [Message length too large: " + std::to_string(msg.size()) +
                 " - Must be 2000 or less.]",
             channel);
        return {};
    }
    json msg_json{{"content", msg}, {"components", json::array()}};
    msg_json["components"].push_back({{"type", 1}, {"components", json::array()}});
    /* {
     *  "content": msg,
     *  "components": [
     *          {
     *              "type", 1,
     *              "components": [
     *              ]
     *          }
     *      ]
     * }
     */
    json my_component{{"type", 2}, {"label", "Test!"}, {"style", 1}, {"custom_id", "click_one"}};
    msg_json["components"][0]["components"].push_back({{"type", 1}, {"components", json::array()}});
    msg_json["components"][0]["components"][0] = my_component;
    qb::log::point(msg_json.dump(2));
    const auto resp = web_ctx_->post(web::Endpoint::channels, channel, msg_json.dump());
    if (!identity_)
    {
        if (json_utils::has_path(resp, "author", "id"))
        {
            identity_ = resp["author"]["id"];
            qb::log::point("Setting identity to: ", *identity_);
        }
        else
        {
            qb::log::data("Note Response", resp.dump(2));
            return resp;
        }
    }
    if (log_loud_) qb::log::data("Response", resp.dump(2));
    return resp;
}

nlohmann::json qb::Bot::send(std::string msg, std::string channel)
{
    if (msg.size() > 2000)
    {
        send("I can't do that. [Message length too large: " + std::to_string(msg.size()) +
                 " - Must be 2000 or less.]",
             channel);
        return {};
    }
    nlohmann::json msg_json{{"content", msg}};
    const auto resp = web_ctx_->post(web::Endpoint::channels, channel, msg_json.dump());
    if (!identity_ && resp.contains("author"))
    {
        identity_ = resp["author"]["id"];
        qb::log::point("Setting identity to: ", *identity_);
    }
    if (log_loud_) qb::log::data("Response", resp.dump(2));
    return resp;
}

nlohmann::json qb::Bot::send_json(const nlohmann::json& msg, const api::Interaction& interaction)
{
    if (msg.value("content", "").size() > 2000 && interaction.channel)
    {
        send("I can't do that. [Message length too large: " + std::to_string(msg["content"].size()) +
                 " - Must be 2000 or less.]",
             interaction.channel->id);
        return {};
    }
    const auto resp = web_ctx_->post(interaction, msg.dump());
    if (!identity_ && resp.contains("author"))
    {
        identity_ = resp["author"]["id"];
        qb::log::point("Setting identity to: ", *identity_);
    }
    if (log_loud_) qb::log::data("Response", resp.dump(2));
    return resp;
}

nlohmann::json qb::Bot::send_json(const nlohmann::json& msg, std::string channel)
{
    if (msg.value("content", "").size() > 2000)
    {
        send("I can't do that. [Message length too large: " + std::to_string(msg["content"].size()) +
                 " - Must be 2000 or less.]",
             channel);
        return {};
    }
    const auto resp = web_ctx_->post(web::Endpoint::channels, channel, msg.dump());
    if (!identity_ && resp.contains("author"))
    {
        identity_ = resp["author"]["id"];
        qb::log::point("Setting identity to: ", *identity_);
    }
    if (log_loud_) qb::log::data("Response", resp.dump(2));
    return resp;
}

bool qb::Bot::dispatch_in(ActionCallback, std::chrono::duration<long>)
{
    // Currently unimplemented. TODO.
    return false;
}

// TODO: these functions should be abstracted away similar to execute_callbacks
void qb::Bot::on_message_id(std::string message_id, qb::ActionCallback action)
{
    if (message_id_callbacks_.find(message_id) == message_id_callbacks_.end())
    {
        message_id_callbacks_.try_emplace(message_id);
    }
    message_id_callbacks_[message_id].emplace_back(std::move(action));
}

void qb::Bot::on_message_reaction(const api::Message& message, BasicAction<api::Reaction> action)
{
    if (message_reaction_callbacks_.find(message.id) == message_reaction_callbacks_.end())
    {
        message_reaction_callbacks_.try_emplace(message.id);
    }
    message_reaction_callbacks_[message.id].emplace_back(std::move(action));
}

void qb::Bot::on_message_interaction(const std::string& key, BasicAction<api::Interaction> action)
{
    if (message_interaction_callbacks_.find(key) == message_interaction_callbacks_.end())
    {
        message_interaction_callbacks_.try_emplace(key);
    }
    message_interaction_callbacks_[key].emplace_back(std::move(action));
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

void qb::Bot::test(const std::string& cmd, const std::string& channel)
{
    qb::log::point("Doing test send with command: ", cmd);
    send_test(cmd, channel);
}

void qb::Bot::list(const std::string&, const std::string& channel)
{
    // We need to build a list of all the JSON keys
    send(qb::messages::keys(qb::parse::concatenate(qb::fileio::skribbl::keys())), channel);
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
    // note that the std::string{all} was added due to weird warning
    // about unspecified results of comparisons. not sure if necessary.
    if (components.size() <= 1 || components[1] == std::string{"all"})
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

void qb::Bot::configure(const std::string& cmd, const api::Channel& channel)
{
    using namespace qb::json_utils;

    if (cmd.size() == 4) return; // `conf`
    const auto command = cmd.substr(5);

    if (command == "1984")
    {
        mode_1984_ = true;
        send("Enabling 1985 mode.", channel.id);
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
        qb::log::err("Encountered read error: ", error.message(), " (", error.category().name(),
                     ':', error.value(), ')');
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
    {
        qb::log::warn("Received opcode 7: Reconnect. Attempting reconnection:");
        attempt_ws_reconnect();
        qb::log::point("Finished reconnection (Cause: 7).");
        break;
    }
    case 9:
    {
        qb::log::warn("Received opcode 9: Invalid session. Attempting reconnection:");
        attempt_ws_reconnect();
        qb::log::point("Finished reconnection (Cause: 9).");
        break;
    }
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

void qb::Bot::attempt_ws_reconnect(bool start_read)
{
    const auto socket_info       = web_ctx_->get(web::Endpoint::gateway_bot);
    const std::string socket_url = socket_info["url"];
    try
    {
        if (ws_) boost::beast::get_lowest_layer(*(ws_->get())).cancel();
    }
    catch (const std::exception& e)
    {
        qb::log::err("Caught while attempting websocket reconnection: ", e.what());
    }
    ws_.reset();
    ws_.emplace(web_ctx_->acquire_websocket(socket_url));

    // This should probably be bundled with ws_ to prevent explicit reset here
    outstanding_write_ = false;

    // Start the asynchronous read loop.
    if (start_read) dispatch_read();

    // Start the asynchronous write loop.
    ping_sender({});
}

void qb::Bot::handle_io_read_str(const std::string& cmd)
{
    using namespace qb::parse;
    qb::log::point("Got i/o read: ", cmd);
    if (cmd == "stop")
    {
        shutdown();
        return;
    }

    if (startswith(cmd, "@"))
    {
        const auto sp = std::find(cmd.begin(), cmd.end(), ' ');
        if (sp != cmd.end())
        {
            const std::string ref{cmd.begin() + 1, sp};
            const std::string acmd{sp + 1, cmd.end()};
            qb::log::point("Note: Found ref: (", ref, ") & command: (", acmd, ")");
            const auto msg = qb::api::Message::create_easy(ref);
            if (!msg)
                qb::log::warn("Failed to parse message qualifier.");
            else
            {
                if (acmd == "bind")
                {
                    qb::log::point("Bound message.");
                    bound_msg_.emplace(msg->id, msg->channel.id, msg->channel.guild, msg->user);
                }
                else if (!handle_command(acmd, *msg, true))
                {
                    qb::log::warn("Could not parse i/o command into an actual command.");
                }
            }
        }
        else
            qb::log::warn("Could not split by space as there was nothing after the space.");
    }
    else if (bound_msg_)
    {
        if (!handle_command(cmd, *bound_msg_, true))
        {
            qb::log::warn("Could not parse i/o command into an actual command.");
        }
    }
    stdin_io_->async_read();
}

void qb::Bot::handle_io_read(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    using namespace qb::parse;
    if (error)
    {
        if (error == boost::asio::error::misc_errors::not_found)
        {
            qb::log::err("Too much data input through STDIN. Bytes read: ", bytes_transferred);
            stdin_io_->cleanse();
        }
        else
        {
            qb::log::err("Encountered i/o read error: ", error.message(), " (",
                         error.category().name(), ':', error.value(), ')');
            return;
        }
    }
    handle_io_read_str(stdin_io_->read(bytes_transferred));
}

void qb::Bot::start()
{
    // Some basic initialization prior to starting any networking calls.
    qb::log::point("Creating a web context.");
    web::context web_context; // todo we need to fix the web context management so we can do it conditionally
    web_context.initialize();
    web_ctx_ = &web_context;
    dead     = false;

    qb::log::point("Creating timer for ping operations.");
    timer_.emplace(*web_context.ioc_ptr(), boost::asio::chrono::milliseconds(hb_interval_ms_));

    qb::log::point("Potentially launching serverside I/O.");
    stdin_io_ = std::make_unique<stdin_io>(
        *web_context.ioc_ptr(),
        std::bind(&qb::Bot::handle_io_read, this, std::placeholders::_1, std::placeholders::_2));
    stdin_io_->async_read();

    // Make API call to Discord /gateway/bot/ to get a WebSocket URL
    auto socket_info             = web_context.get(web::Endpoint::gateway_bot);
    const std::string socket_url = socket_info["url"];
    qb::log::data("Socket information", socket_info.dump(2));

    // Acquire a websocket connection to the URL.
    ws_.emplace(web_context.acquire_websocket(socket_url));

    // Start the asynchronous read loop.
    dispatch_read();

    // Start the asynchronous write loop.
    ping_sender({});

    /**
     * Any delayed startup should go here.
     */
    qb::Hangman hangman;
    hangman.register_actions(actions_);

    qb::QueueComponent queues;
    queues.register_actions(actions_);

    qb::SprintComponent sprints;
    sprints.register_actions(actions_);

    qb::TodoComponent todos;
    todos.register_actions(actions_);

    qb::CommandsComponent commands;
    commands.register_actions(actions_);

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
    if (stdin_io_) stdin_io_->close();
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
    web_ctx_->shutdown(false);
    qb::log::point("Shutdown completed.");
}

