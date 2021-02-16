#include "gamequeue.hpp"

#include "bot.hpp"
#include "utils/fileio.hpp"
#include "utils/parse.hpp"
#include "utils/utils.hpp"

void qb::QueueComponent::register_actions(Actions<>& actions)
{
    using namespace std::placeholders;
    register_all(
        actions,
        std::make_pair("startqueue", (ActionCallback)std::bind(&qb::QueueComponent::add_queue, this, _1, _2, _3)),
        std::make_pair("stopqueue", (ActionCallback)std::bind(&qb::QueueComponent::remove_queue, this, _1, _2, _3))
    );
}

qb::Result qb::QueueComponent::add_queue(const std::string& cmd, const api::Message& msg, Bot& bot)
{
    const auto& channel = msg.channel;
    std::vector<std::string> tokens = qb::parse::split(cmd);
    if( tokens.size() != 4)
    {
        bot.send("Incorrect number of parameters given. Proper format : !qb queue <game> <queue_name> <max_size>", channel);
        return qb::Result::ok();
    }
    else
    {
        const auto game = tokens[1];
        const auto name = tokens[2];
        const auto max_size = std::stoi(tokens[3]);

        active_queues.emplace(name, Queue(name, msg.guild, msg.channel, game, max_size));
        send_yn_message(bot, name, "Queueing a game of " + game + " called " + name + " with 0 players. (Max " + tokens[3] + ")", channel);
        return qb::Result::ok();
    }
}

qb::Result qb::QueueComponent::remove_queue(const std::string& cmd, const api::Message& msg, Bot& bot)
{
    const auto& channel = msg.channel;
    std::vector<std::string> tokens = qb::parse::split(cmd);
    if(tokens.size() != 2)
    {
        bot.send("Incorrect number of parameters given. Proper format : !qb dequeue <name>", channel);
        return qb::Result::ok();
    }
    else
    {
        const auto name = tokens[1];
        active_queues.erase(name);
        bot.send("Queue called " + name + " removed.", channel);
        return qb::Result::ok();
    }
}

nlohmann::json qb::QueueComponent::send_yn_message(Bot& bot, const std::string& name, const std::string& message, const std::string& channel)
{
    qb::log::point("A queue message.");
    const auto resp = bot.send(message, channel);
    if(resp.empty()) return {};
    bot.on_message_id(resp["id"], bind_action(&qb::QueueComponent::add_yn_reaction, this));
    return resp;

}
qb::Result qb::QueueComponent::add_yn_reaction(const std::string& name, const std::string& message_id, const api::Message& message, Bot& bot)
{
    if (const auto& yes_emote = qb::parse::emote_snowflake(qb::fileio::get_emote("yes")); yes_emote)
    {
        bot.get_context()->put(qb::endpoints::reaction(message.channel, message_id, *yes_emote), {});
    }
    if (const auto& no_emote = qb::parse::emote_snowflake(qb::fileio::get_emote("no")); no_emote)
    {
        bot.get_context()->put(qb::endpoints::reaction(message.channel, message_id, *no_emote), {});
    }

    bot.on_message_reaction(message, [this, em = qb::fileio::get_emote("yes"), message_id, message](
                                             const std::string&, const api::Reaction& reaction, Bot& bot, std::string& name) {
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
            if (qb::parse::compare_emotes(em, reaction.emoji))
            {
                qb::log::point("Editing message");
                nlohmann::json new_message;
                new_message["content"] = "Queueing a game of " + active_queues[name] + " called " + name + " with 0 players. (Max " + tokens[3] + ")";
                bot.get_context()->patch(message.endpoint(), new_message );
                return qb::Result::Value::Ok;
            }
            return qb::Result::Value::PersistCallback;
        });
}