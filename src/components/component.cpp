#include "component.hpp"

#include "bot.hpp"
#include "utils/fileio.hpp"
#include "web/strings.hpp"
#include "utils/parse.hpp"

nlohmann::json qb::Component::send_removable_message(Bot& bot, const std::string& message, const std::string& channel)
{
    qb::log::point("Sending a removable message.");
    const auto resp = bot.send(message, channel);
    if (resp.empty()) return {};
    bot.on_message_id(resp["id"], bind_action(&qb::Component::add_delete_reaction, this));
    return resp;
}

qb::Result qb::Component::add_delete_reaction(const std::string& message_id, const api::Message& message, Bot& bot)
{
    // temporary until we have proper api wrappers (hah)
    if (const auto& emote = qb::parse::emote_snowflake(qb::fileio::get_emote("remove_message")); emote)
    {
        bot.get_context()->put(qb::endpoints::reaction(message.channel, message_id, *emote), {});
        qb::log::point("Added remove_message reaction to message ID ", message_id);
        return qb::Result::ok();
    }
    // TODO return bad result
    qb::log::point("Did not add reaction to message ID ", message_id,
                   " as the remove_message emote is unavailable.");
    return qb::Result::ok();
}
