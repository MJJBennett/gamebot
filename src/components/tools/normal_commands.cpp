#include "normal_commands.hpp"

#include "bot.hpp"
#include "utils/fileio.hpp"
#include "utils/parse.hpp"
#include "utils/utils.hpp"

void qb::CommandsComponent::register_actions(Actions<>& actions)
{
    using namespace std::placeholders;
    register_all(actions, std::make_pair("poll", (BasicAction<api::Message>)std::bind(
                                                     &qb::CommandsComponent::add_poll, this, _1, _2, _3)));
}

qb::Result qb::CommandsComponent::add_poll(const std::string& cmd, const api::Message& msg, Bot& bot)
{
    const auto pos             = std::find(cmd.begin(), cmd.end(), ':');
    const std::string question = [&cmd, pos]() {
        if (pos == cmd.end()) return std::string{};
        return std::string{cmd.begin(), pos};
    }();
    const auto start   = (pos == cmd.end()) ? cmd.begin() : (pos + 1);
    const auto options = qb::parse::split(std::string{start, cmd.end()}, ';');
    if (options.size() < 1 || options.size() > 5)
    {
        send_removable_message(bot, "Sorry, currently we only support polls of 1-5 options.", msg.channel.id);
    }
    else
    {
        std::vector<qb::Button> buttons;
        size_t i = 0;
        for (const auto& q : options)
        {
            const auto prompt = qb::parse::trim(q);
            buttons.emplace_back(std::to_string(++i), prompt);
        }
        const auto resp = send_buttons(bot, question == "" ? "Poll!" : question, buttons, msg.channel);
        if (!resp.empty())
        {
            bot.on_message_interaction(
                resp["id"], [channel = msg.channel.id](const std::string& msgid,
                                                       const api::Interaction& interaction, Bot& bot) {
                    bot.send("Our friend " + interaction.user.username.value_or("someone") + " voted!", channel);
                    return qb::Result::Value::PersistCallback;
                });
        }
    }

    auto endpoint = msg.endpoint();
    bot.get_context()->del(endpoint);

    return qb::Result::ok();
}

void qb::CommandsComponent::dump_debug() const
{
    qb::log::point("Dumping CommandsComponent debug information. [START]");
    qb::log::point("Finished dumping CommandsComponent debug information. [END]");
}
