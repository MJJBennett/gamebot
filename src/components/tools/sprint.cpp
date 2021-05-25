#include "sprint.hpp"

#include "bot.hpp"
#include "utils/fileio.hpp"
#include "utils/parse.hpp"
#include "utils/utils.hpp"

#include <boost/asio/steady_timer.hpp>

std::string qb::Sprint::to_str() const
{
    return "Running a sprint (" + get_name() + ") with **" +
           std::to_string(users.size()) + "** users. React with " +
           qb::fileio::get_emote("yes") + " to join the sprint! " +
           "\n**Sprint starting now, and going for: " + time_str_ + "**";
}

void qb::SprintComponent::register_actions(Actions<>& actions)
{
    using namespace std::placeholders;
    register_all(
        actions,
        std::make_pair("sprint", (ActionCallback)std::bind(&qb::SprintComponent::add_sprint, this, _1, _2, _3)),
        std::make_pair("db:sprint", bind_message_action(
                                       [](qb::SprintComponent* obj, const std::string&, const api::Message&, Bot&) {
                                           obj->dump_debug();
                                           return qb::Result::Value::Ok;
                                       },
                                       this)));
}

qb::Result qb::SprintComponent::add_sprint(const std::string& cmd, const api::Message& msg, Bot& bot)
{
    const auto& channel = msg.channel;

    // IIFE, very nice
    Sprint new_sprint = [&]() {
        auto [args, numeric_args, duration_args, durations] = qb::parse::decompose_command(cmd);
        qb::log::point("Sprint creation: \n\tFound ", args.size(), " arguments\n\tFound ",
                       numeric_args.size(), " numeric args\n\tFound ", duration_args.size(),
                       " duration args");
        std::optional<std::string> name;
        if (args.size() >= 1)
        {
            name = qb::parse::concatenate(args, " ");
        }
        std::chrono::duration<long> time = std::chrono::minutes(15);
        std::string time_str = "15m";
        if (duration_args.size() != 0)
        {
            if (duration_args[0].count() < 10)
                send_removable_message(bot, "Sorry, please choose a time that is greater than 10s. Using default time.", channel);
            else {
                time = duration_args[0];
                time_str = durations[0];
            }
        }
        return Sprint(name, time, time_str);
    }();

    auto endpoint = msg.endpoint();
    bot.get_context()->del(endpoint);
    send_yn_message(std::move(new_sprint), bot, new_sprint.to_str(), channel);
    return qb::Result::ok();
}

// TODO should probably take Sprint and not Sprint&
nlohmann::json qb::SprintComponent::send_yn_message(Sprint&& sprint,
                                                   Bot& bot,
                                                   const std::string& message,
                                                   const std::string& channel)
{
    qb::log::point("A sprint message.");
    const auto resp = send_removable_message(bot, message, channel);
    if (resp.empty()) return {};
    active_sprints.emplace(resp["id"], std::move(sprint));
    bot.on_message_id(resp["id"], bind_message_action(&qb::SprintComponent::add_yn_reaction, this));
    return resp;
}

qb::Result qb::SprintComponent::add_yn_reaction(const std::string& message_id, const api::Message& message, Bot& bot)
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
            if (active_sprints.find(message_id) != active_sprints.end())
            {
                auto& q = active_sprints.at(message_id);
                if (!q.timer_)
                {
                    // this is a chrono duration so it should be correct by default, actually
                    q.timer_.emplace(*(bot.get_context()->ioc_ptr()), q.time_);
                    q.timer_->async_wait([this, &bot, reaction](const boost::system::error_code& error) {
                        if (error)
                        {
                            qb::log::err("Found some kind of error with async_wait.");
                            qb::log::err(error.message());
                            return;
                        }
                        this->end_sprint(reaction.message_id, reaction, bot);
                    });
                }
            }
            else
                qb::log::warn("Couldn't find the message in sprints for some reason. (#1)");

            qb::log::point("> > Did not edit message, as it was sent by the bot.");
            return qb::Result::Value::PersistCallback;
        }
        if (qb::parse::compare_emotes(qb::fileio::get_emote("remove_message"), reaction.emoji))
        {
            qb::log::point("SprintComponent using Delete emoji to end sprint.");
            return *end_sprint(message_id, reaction, bot).val;
        }
        if (qb::parse::compare_emotes(em, reaction.emoji))
        {
            if (count == 0)
            {
                auto& users    = active_sprints.at(message_id).users;
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
                active_sprints.at(message.id).users.push_back(reaction.user.id);
                qb::log::point("Editing message");

                if (active_sprints.empty())
                {
                    qb::log::point("Oh no, active sprints is empty");
                }
                auto& q     = active_sprints.at(message_id);
                auto& users = active_sprints.at(message_id).users;
            }
            nlohmann::json new_message;
            new_message["content"] = active_sprints.at(message.id).to_str();
            bot.get_context()->patch(message.endpoint(), new_message.dump());
            return qb::Result::Value::PersistCallback;
        }
        return qb::Result::Value::PersistCallback;
    });
    return qb::Result::ok();
}

qb::Result qb::SprintComponent::end_sprint(const std::string& message_id, const api::Reaction& reaction, Bot& bot)
{
    if (active_sprints.find(message_id) == active_sprints.end())
    {
        // this is probably fine and not a concern
        qb::log::warn("Already deleted sprint at message ID ", message_id);
        return qb::Result::Value::Ok;
    }
    auto& q     = active_sprints.at(message_id);
    auto& users = active_sprints.at(message_id).users;
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
    ss << "\nSprint completed (" + q.get_name() + ")!";
    send_removable_message(bot, ss.str(), reaction.channel);
    bot.get_context()->del(qb::endpoints::message(reaction.channel, message_id));
    active_sprints.erase(message_id);
    return qb::Result::Value::Ok;
}

void qb::SprintComponent::dump_debug() const
{
    qb::log::point("Dumping SprintComponent debug information. [START]");
    qb::log::point("SprintComponent contains ", active_sprints.size(), " active sprints.");
    qb::log::point("Finished dumping SprintComponent debug information. [END]");
}
