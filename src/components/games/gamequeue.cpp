#include "gamequeue.hpp"

#include "bot.hpp"
#include "utils/fileio.hpp"
#include "utils/parse.hpp"
#include "utils/utils.hpp"

std::string qb::Queue::to_str() const
{
    return "Queueing a game of " + game_ + " called " + name_ + " with **" + std::to_string(users.size()) + "** players. (Max **" + std::to_string(max_size_) + "**)";
}

void qb::QueueComponent::register_actions(Actions<>& actions)
{
    using namespace std::placeholders;
    register_all(
        actions,
        std::make_pair("queue", (ActionCallback)std::bind(&qb::QueueComponent::add_queue, this, _1, _2, _3))
    );
}

qb::Result qb::QueueComponent::add_queue(const std::string& cmd, const api::Message& msg, Bot& bot)
{
    const auto& channel = msg.channel;
    std::vector<std::string> tokens = qb::parse::split(cmd);
    if( tokens.size() != 5)
    {
        bot.send("Incorrect number of parameters given. Proper format : !qb queue <game> <queue_name> <max_size> <time>", channel);
        return qb::Result::ok();
    }
    else
    {
        const auto game = tokens[1];
        const auto name = tokens[2];
        const auto max_size = std::stoi(tokens[3]);
        const auto time = std::stoi(tokens[4]);
        Queue new_queue = Queue(name, game, max_size, time);
        auto endpoint = msg.endpoint();
        bot.get_context()->del(endpoint);
        send_yn_message(new_queue, bot, "Queueing a game of " + game + " called " + name + " with **0** players. (Max players: " + tokens[3] + ". Time remaining: " + tokens[4] + ")", channel);
        return qb::Result::ok();

    }
}

nlohmann::json qb::QueueComponent::send_yn_message(Queue& queue, Bot& bot, const std::string& message, const std::string& channel)
{
    qb::log::point("A queue message.");
    const auto resp = send_removable_message(bot, message, channel);
    if(resp.empty()) return {};
    active_queues.emplace(resp["id"], queue);
    bot.on_message_id(resp["id"], bind_message_action(&qb::QueueComponent::add_yn_reaction, this));
    return resp;

}
qb::Result qb::QueueComponent::add_yn_reaction( const std::string& message_id, const api::Message& message, Bot& bot)
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
            if (qb::parse::compare_emotes(em, reaction.emoji))
            {
                if(count == 0){
                    auto& users = active_queues.at(message_id).users;
                    const auto itr = std::remove(users.begin(), users.end(), reaction.user.id);
                    if (itr == users.end()) {
                    qb::log::point("The user ", reaction.user.id, " was not registered!");
                    return qb::Result::Value::PersistCallback;
                    }
                    users.erase(itr, users.end());
                }
                else{
                    active_queues.at(message.id).users.push_back(reaction.user.id);
                    qb::log::point("Editing message");
                    
                    if(active_queues.empty())
                    {
                        qb::log::point("Oh no, active queues is empty");
                    }
                    if(active_queues.at(message.id).users.size() == active_queues.at(message_id).max_size_)
                    {
                        auto& users = active_queues.at(message_id).users;
                        std::stringstream ss;
                        for (const auto& i: users){
                            ss << "<@" << i << "> ";
                        }
                        ss << "\nQueue finished! Please get ready to play " << active_queues.at(message.id).game_ << " with " <<  active_queues.at(message.id).name_ << ".";
                        send_removable_message(bot, ss.str(), message.channel);
                        bot.get_context()->del(message.endpoint());
                        active_queues.erase(message_id);
                        return qb::Result::Value::Ok;
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