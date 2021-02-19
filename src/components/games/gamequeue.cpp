#include "gamequeue.hpp"

#include "bot.hpp"
#include "utils/fileio.hpp"
#include "utils/parse.hpp"
#include "utils/utils.hpp"

std::string qb::Queue::to_str() const
{
    std::string max_players = max_size_ ? (" (Max **" + std::to_string(*max_size_) + "**)") : "";
    return "Queueing a game of " + get_game() + " called " + get_name() + " with **" +
           std::to_string(users.size()) + "** players." + max_players + " React with " +
           qb::fileio::get_emote("yes") + " to join the queue!";
}

void qb::QueueComponent::register_actions(Actions<>& actions)
{
    using namespace std::placeholders;
    register_all(actions, std::make_pair("queue", (ActionCallback)std::bind(&qb::QueueComponent::add_queue,
                                                                            this, _1, _2, _3)));
}

qb::Result qb::QueueComponent::add_queue(const std::string& cmd, const api::Message& msg, Bot& bot)
{
    const auto& channel = msg.channel;

    Queue new_queue = [&]() {
        auto [args, numeric_args, duration_args] = qb::parse::decompose_command(cmd);
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
        if (duration_args.size() != 0)
        {
            time = duration_args[0];
        }
        return Queue(name, game, max_size, time);
    }();

    auto endpoint = msg.endpoint();
    bot.get_context()->del(endpoint);
    send_yn_message(new_queue, bot, new_queue.to_str(), channel);
    return qb::Result::ok();
}

nlohmann::json qb::QueueComponent::send_yn_message(Queue& queue,
                                                   Bot& bot,
                                                   const std::string& message,
                                                   const std::string& channel)
{
    qb::log::point("A queue message.");
    const auto resp = send_removable_message(bot, message, channel);
    if (resp.empty()) return {};
    active_queues.emplace(resp["id"], queue);
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
        const auto& bID = bot.idref(); // ALSO A HACK
        if (!bot.idref())
        {
            qb::log::point("> > Did not edit message, as the bot has no ID.");
            bot.idref() = reaction.user.id;
            return qb::Result::Value::PersistCallback;
        }
        if (reaction.user.id == *bot.idref())
        {
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
                if (q.max_size_ && users.size() == *(q.max_size_))
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

qb::Result qb::QueueComponent::end_queue(const std::string& message_id, const api::Reaction reaction, Bot& bot)
{
    if (active_queues.find(message_id) == active_queues.end())
    {
        qb::log::warn("Already deleted queue at message ID ", message_id);
        return qb::Result::Value::Ok;
    }
    auto& q     = active_queues.at(message_id);
    auto& users = active_queues.at(message_id).users;
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
