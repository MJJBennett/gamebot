#include "component.hpp"

#include "bot.hpp"
#include "utils/fileio.hpp"
#include "web/strings.hpp"

nlohmann::json qb::Component::send_removable_message(Bot& bot, const std::string& message, const std::string& channel)
{
    const auto resp = bot.send(message, channel);
    if (resp.empty()) return {};
    bot.on_message_id(resp["id"], bind_action(&qb::Component::add_delete_reaction, this));
    return resp;
}

qb::Result qb::Component::add_delete_reaction(const std::string& message_id, const api::Message& message, Bot& bot)
{
    // temporary until we have proper api wrappers (hah)
    if (const auto& emote = qb::fileio::get_emote("remove_message"); emote != "none")
    {
        bot.get_context()->put(qb::endpoints::reaction(message.channel, message_id, emote), {});
        return qb::Result::ok();
    }
    // TODO return bad result
    return qb::Result::ok();
}
