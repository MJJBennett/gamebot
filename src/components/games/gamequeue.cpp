#include "gamequeue.hpp"

#include "bot.hpp"
#include "utils/fileio.hpp"
#include "utils/parse.hpp"
#include "utils/utils.hpp"

#include <boost/asio/steady_timer.hpp>

std::string qb::Queue::to_str() const
{
    std::string max_players = max_size_ ? (" (Max **" + std::to_string(*max_size_) + "**)") : "";
    std::string ret_str = "Queueing a game of " + get_game() + " called " + get_name() + " with **" +
           std::to_string(users.size()) + "** players." + max_players + " React with " +
           qb::fileio::get_emote("yes") + " to join the queue!";
    if (time_str_) ret_str += "\n**Starting in: " + get_time_str() + "**";
    return ret_str;
}

void qb::QueueComponent::register_actions(Actions<>& actions)
{
    using namespace std::placeholders;
    register_all(
        actions,
        std::make_pair("queue", (ActionCallback)std::bind(&qb::QueueComponent::add_queue, this, _1, _2, _3)),
        std::make_pair("db:queue", bind_message_action(
                                       [](qb::QueueComponent* obj, const std::string&, const api::Message&, Bot&) {
                                           obj->dump_debug();
                                           return qb::Result::Value::Ok;
                                       },
                                       this)));
}

qb::Result qb::QueueComponent::add_queue(const std::string& cmd, const api::Message& msg, Bot& bot)
{
    const auto& channel = msg.channel;

    // IIFE, very nice
    Queue new_queue = [&]() {
        auto [args, numeric_args, duration_args, durations] = qb::parse::decompose_command(cmd);
        qb::log::point("Queue creation: \n\tFound ", args.size(), " arguments\n\tFound ",
                       numeric_args.size(), " numeric args\n\tFound ", duration_args.size(),
                       " duration args");
        std::optional<std::string> name;
        std::optional<std::string> game;
        if (args.size() == 1)
        {
            game = args[0];
        }
        else if (args.size() > 1)
        {
            game = args[0];
            name = args[1];
        }
        std::optional<int> max_size;
        if (numeric_args.size() != 0)
        {
            max_size = numeric_args[0];
        }
        std::optional<std::chrono::duration<long>> time;
        std::optional<std::string> time_str;
        if (duration_args.size() != 0)
        {
            if (duration_args[0].count() < 10)
                send_removable_message(bot, "Sorry, please choose a time that is greater than 10s.", channel);
            else {
                time = duration_args[0];
                time_str = durations[0];
            }
        }
        return Queue(name, game, max_size, time, time_str);
    }();

    auto endpoint = msg.endpoint();
    bot.get_context()->del(endpoint);
    send_yn_message(std::move(new_queue), bot, new_queue.to_str(), channel.id);
    return qb::Result::ok();
}

// TODO should probably take Queue and not Queue&
nlohmann::json qb::QueueComponent::send_yn_message(Queue&& queue,
                                                   Bot& bot,
                                                   const std::string& message,
                                                   const std::string& channel)
{
    qb::log::point("A queue message.");
    const auto resp = send_removable_message(bot, message, channel);
    if (resp.empty()) return {};
    active_queues.emplace(resp["id"], std::move(queue));
    bot.on_message_id(resp["id"], bind_message_action(&qb::QueueComponent::add_yn_reaction, this));
    return resp;
}

qb::Result qb::QueueComponent::add_yn_reaction(const std::string& message_id, const api::Message& message, Bot& bot)
{
    if (const auto& yes_emote = qb::parse::emote_snowflake(qb::fileio::get_emote("yes")); yes_emote)
    {
        bot.get_context()->put(qb::endpoints::reaction(message.channel, message_id, *yes_emote), {});
    }

    bot.on_message_reaction(message, [this, em = qb::fileio::get_emote("yes"), message_id, message](
                                         const std::string&, const api::Reaction& reaction, Bot& bot, int count) {
        qb::log::point("> Checking if reaction: ", reaction.to_string(),
                       " is the correct reaction to edit the message: ", message_id);
        auto& bID = bot.idref(); // ALSO A HACK
        if (!bID)
        {
            qb::log::point("> > Did not edit message, as the bot has no ID.");
            bID = reaction.user.id;
        }
        if (reaction.user.id == *bID)
        {
            /**
             * We now need to add the callback for this message.
             * This requires adding an async callback to the io_context
             */
            if (active_queues.find(message_id) != active_queues.end())
            {
                auto& q = active_queues.at(message_id);
                if (q.time_ && !q.timer_)
                {
                    // this is a chrono duration so it should be correct by default, actually
                    q.timer_.emplace(*(bot.get_context()->ioc_ptr()), *q.time_);
                    q.timer_->async_wait([this, &bot, reaction](const boost::system::error_code& error) {
                        if (error)
                        {
                            qb::log::err("Found some kind of error with async_wait.");
                            qb::log::err(error.message());
                            return;
                        }
                        this->end_queue(reaction.message_id, reaction, bot);
                    });
                }
            }
            else
                qb::log::warn("Couldn't find the message in queues for some reason. (#1)");

            qb::log::point("> > Did not edit message, as it was sent by the bot.");
            return qb::Result::Value::PersistCallback;
        }
        if (qb::parse::compare_emotes(qb::fileio::get_emote("remove_message"), reaction.emoji))
        {
            qb::log::point("QueueComponent using Delete emoji to end queue.");
            return *end_queue(message_id, reaction, bot).val;
        }
        if (qb::parse::compare_emotes(em, reaction.emoji))
        {
            if (count == 0)
            {
                auto& users    = active_queues.at(message_id).users;
                const auto itr = std::remove(users.begin(), users.end(), reaction.user.id);
                if (itr == users.end())
                {
                    qb::log::point("The user ", reaction.user.id, " was not registered!");
                    return qb::Result::Value::PersistCallback;
                }
                users.erase(itr, users.end());
            }
            else
            {
                active_queues.at(message.id).users.push_back(reaction.user.id);
                qb::log::point("Editing message");

                if (active_queues.empty())
                {
                    qb::log::point("Oh no, active queues is empty");
                }
                auto& q     = active_queues.at(message_id);
                auto& users = active_queues.at(message_id).users;
                if (q.max_size_ && users.size() == static_cast<size_t>(*(q.max_size_)))
                {
                    return *end_queue(message_id, reaction, bot).val;
                }
            }
            nlohmann::json new_message;
            new_message["content"] = active_queues.at(message.id).to_str();
            bot.get_context()->patch(message.endpoint(), new_message.dump());
            return qb::Result::Value::PersistCallback;
        }
        return qb::Result::Value::PersistCallback;
    });
    return qb::Result::ok();
}

qb::Result qb::QueueComponent::end_queue(const std::string& message_id, const api::Reaction& reaction, Bot& bot)
{
    if (active_queues.find(message_id) == active_queues.end())
    {
        // this is probably fine and not a concern
        qb::log::warn("Already deleted queue at message ID ", message_id);
        return qb::Result::Value::Ok;
    }
    auto& q     = active_queues.at(message_id);
    auto& users = active_queues.at(message_id).users;
    if (q.timer_)
    {
        q.timer_->cancel();
        q.timer_.reset();
    }
    std::stringstream ss;
    for (const auto& i : users)
    {
        ss << "<@" << i << "> ";
    }
    ss << "\nQueue finished! Please get ready to play " << (q.game_ ? *q.game_ : "a game")
       << " with " << (q.name_ ? *q.name_ : "friends!") << "!";
    send_removable_message(bot, ss.str(), reaction.channel);
    bot.get_context()->del(qb::endpoints::message(reaction.channel, message_id));
    active_queues.erase(message_id);
    return qb::Result::Value::Ok;
}

void qb::QueueComponent::dump_debug() const
{
    qb::log::point("Dumping QueueComponent debug information. [START]");
    qb::log::point("QueueComponent contains ", active_queues.size(), " active queues.");
    qb::log::point("Finished dumping QueueComponent debug information. [END]");
}
