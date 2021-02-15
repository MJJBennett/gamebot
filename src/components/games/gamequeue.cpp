#include "gamequeue.hpp"

#include "bot.hpp"
#include "utils/fileio.hpp"
#include "utils/parse.hpp"
#include "utils/utils.hpp"

void qb::QueueComponent::register_actions(Actions& actions)
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
        bot.send("Queueing a game of " + game + " called " + name + " with 0 players. (Max " + tokens[3] + ")", channel);
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
        const auto game = tokens[1];
        active_queues.erase(game);
        bot.send("Queue for game of " + game + " removed.", channel);
        return qb::Result::ok();
    }
}