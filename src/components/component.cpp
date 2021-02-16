#include "component.hpp"

#include "bot.hpp"
#include "utils/fileio.hpp"
#include "utils/parse.hpp"
#include "web/strings.hpp"

nlohmann::json qb::Component::send_removable_message(Bot& bot, const std::string& message, const std::string& channel)
{
    qb::log::point("Sending a removable message.");
    const auto resp = bot.send(message, channel);
    if (resp.empty()) return {};
    bot.on_message_id(resp["id"], bind_action(&qb::Component::add_delete_reaction, this));
    return resp;
}
nlohmann::json qb::Component::send_yn_message(Bot& bot, const std::string& message, const std::string& channel)
{
    qb::log::point("A queue message.");
    const auto resp = bot.send(message, channel);
    if(resp.empty()) return {};
    bot.on_message_id(resp["id"], bind_action(&qb::Component::add_yn_reaction, this));
    return resp;

}
qb::Result qb::Component::add_yn_reaction(const std::string& message_id, const api::Message& message, Bot& bot)
{
    if (const auto& yes_emote = qb::parse::emote_snowflake(qb::fileio::get_emote("yes")); yes_emote)
    {
        bot.get_context()->put(qb::endpoints::reaction(message.channel, message_id, *yes_emote), {});
    }
    if (const auto& no_emote = qb::parse::emote_snowflake(qb::fileio::get_emote("no")); no_emote)
    {
        bot.get_context()->put(qb::endpoints::reaction(message.channel, message_id, *no_emote), {});
    }

    bot.on
}
qb::Result qb::Component::add_delete_reaction(const std::string& message_id, const api::Message& message, Bot& bot)
{
    // temporary until we have proper api wrappers (hah)
    if (const auto& emote = qb::parse::emote_snowflake(qb::fileio::get_emote("remove_message")); emote)
    {
        bot.get_context()->put(qb::endpoints::reaction(message.channel, message_id, *emote), {});
        qb::log::point("Added remove_message reaction to message ID ", message_id);

        // Now we need to monitor this message and delete it when the remove_message emote
        // is added as a reaction to the message.
        bot.on_message_reaction(message, [em = qb::fileio::get_emote("remove_message"), message_id, message](
                                             const std::string&, const api::Reaction& reaction, Bot& bot) {
            qb::log::point("> Checking if reaction: ", reaction.to_string(),
                           " is the correct reaction to remove the message: ", message_id);
            const auto& bID = bot.idref(); // ALSO A HACK
            if (!bot.idref())
            {
                qb::log::point("> > Did not remove message, as the bot has no ID.");
                bot.idref() = reaction.user.id;
                return qb::Result::Value::PersistCallback;
            }
            if (reaction.user.id == *bot.idref())
            {
                qb::log::point("> > Did not remove message, as it was sent by the bot.");
                return qb::Result::Value::PersistCallback;
            }
            if (qb::parse::compare_emotes(em, reaction.emoji))
            {
                qb::log::point("Removing message; compared true.");
                bot.get_context()->del(message.endpoint());
                return qb::Result::Value::Ok;
            }
            return qb::Result::Value::PersistCallback;
        });

        return qb::Result::ok();
    }
    // TODO return bad result
    qb::log::point("Did not add reaction to message ID ", message_id,
                   " as the remove_message emote is unavailable.");
    return qb::Result::ok();
}
