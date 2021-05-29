#include "normal_commands.hpp"

#include "bot.hpp"
#include "components/config.hpp"
#include "utils/fileio.hpp"
#include "utils/parse.hpp"
#include "utils/utils.hpp"

struct BasicCommand
{
    static int parse_flags(const std::string& fs)
    {
        int i = 0;
        for (const auto& f : qb::parse::split(fs, '|'))
        {
            if (f == "link")
                i |= 0b1;
            else if (f == "del")
                i |= 0b10;
        }
        return i;
    }
    std::string key;
    std::string resp;
    int flags = 0;
    std::string second;
};

void qb::CommandsComponent::register_actions(Actions<>& actions)
{
    using namespace std::placeholders;
    register_all(actions, std::make_pair("poll", (BasicAction<api::Message>)std::bind(
                                                     &qb::CommandsComponent::add_poll, this, _1, _2, _3)));
    const auto cmds = qb::fileio::readlines_nonempty(qb::config::commands_file());
    std::vector<BasicCommand> commands;
    for (const auto& c : cmds)
    {
        if (c[0] == '#') continue;
        const auto cmd = qb::parse::xsv(c, ',');
        if (cmd.size() >= 3)
        {
            auto bcr = BasicCommand{qb::parse::trim(cmd[1]), qb::parse::trim(cmd[2]),
                                    BasicCommand::parse_flags(qb::parse::trim(cmd[0])), ""};
            commands.emplace_back(std::move(bcr));
        }
        if (cmd.size() >= 4)
        {
            commands.back().second = cmd[3];
        }
    }
    for (const auto& cmd : commands)
    {
        if (cmd.flags & 0b00000001)
        {
            actions.try_emplace(
                cmd.key, (BasicAction<api::Message>)[
                    s = cmd.resp, label = cmd.second, del = (bool)(cmd.flags & 0b10)
                ](const std::string&, const api::Message& msg, Bot& bot) {
                    // we actually have to have an upper-tier actionrow...
                    qb::log::point("Responding to a link command.");
                    const nlohmann::json b{
                        {"type", 2}, {"label", (label.size() == 0 ? "link" : label)}, {"style", 5}, {"url", s}};
                    auto f = nlohmann::json{{"content", "** **"}, {"components", nlohmann::json::array()}};
                    auto inner = nlohmann::json{{"type", 1}, {"components", nlohmann::json::array()}};
                    inner["components"].push_back(b);
                    f["components"].push_back(inner);
                    qb::log::point("Responding with: ", f.dump(2));
                    bot.send_json(f, msg.channel.id);
                    qb::log::point("Finished responding to a link command.");
                    if (del && msg.id != "")
                    {
                        auto endpoint = msg.endpoint();
                        bot.get_context()->del(endpoint);
                    }
                    return qb::Result::ok();
                });
            qb::log::point("Registered command for key: ", cmd.key);
        }
        else
        {
            actions.try_emplace(
                cmd.key,
                (BasicAction<api::Message>)[ s = cmd.resp, del = (bool)(cmd.flags & 0b10) ](
                    const std::string&, const api::Message& msg, Bot& bot) {
                    bot.send_json({{"content", s}}, msg.channel.id);
                    if (del && msg.id != "")
                    {
                        auto endpoint = msg.endpoint();
                        bot.get_context()->del(endpoint);
                    }
                    return qb::Result::ok();
                });
            qb::log::point("Registered command for key: ", cmd.key);
        }
    }
}

qb::Result qb::CommandsComponent::add_poll(const std::string& precmd, const api::Message& msg, Bot& bot)
{
    const auto cmd             = qb::parse::get_command_specifier(precmd);
    const auto pos             = std::find(cmd.begin(), cmd.end(), ':');
    const std::string question = [&cmd, pos]() {
        if (pos == cmd.end()) return std::string{};
        return std::string{cmd.begin(), pos};
    }();
    const auto start   = (pos == cmd.end()) ? cmd.begin() : (pos + 1);
    const auto options = qb::parse::split(std::string{start, cmd.end()}, ';');
    if (options.size() < 1 || options.size() > 5)
    {
        send_removable_message(bot, "Sorry, currently we only support polls of 1-5 options.",
                               msg.channel.id);
    }
    else
    {
        std::vector<qb::Button> buttons;
        std::vector<Poll::Option> ovec;
        size_t i = 0;
        for (const auto& q : options)
        {
            const auto prompt = qb::parse::trim(q);
            const auto num    = std::to_string(++i);
            buttons.emplace_back(num, prompt);
            ovec.emplace_back(Poll::Option{prompt, num, 0});
        }
        const auto resp = send_buttons(bot, question == "" ? "Poll!" : question, buttons, msg.channel);
        if (!resp.empty())
        {
            polls.emplace(resp["id"], Poll{{}, std::move(ovec)});
            bot.on_message_interaction(resp["id"], [channel = msg.channel.id, &p = polls](
                                                       const std::string& msgid,
                                                       const api::Interaction& interaction, Bot& bot) {
                const std::string& un = interaction.user.username.value_or("someone");
                if (p.find(msgid) == p.end())
                    return qb::Result::Value::Ok; // should never happen :)
                const auto resp = [&un, interaction](Poll& poll) {
                    if (!poll.voters.insert(un).second)
                    {
                        return api::Interaction::response()
                            .with_content("Our friend " + un +
                                          " tried to vote multiple times! I *will* ban you. Scum.")
                            .with_flags(64)
                            .as_json();
                    }
                    const auto& cid = interaction.data.custom_id;
                    if (cid.size() == 0 || cid[0] < 49 || cid[0] >= (49 + 5))
                    {
                        return api::Interaction::response()
                            .with_type(4)
                            .with_flags(64)
                            .with_content("Sorry, that failed.")
                            .as_json();
                    }
                    int cidval   = cid[0] - 49;
                    auto& option = poll.options.at(cidval);
                    option.votes += 1;
                    return api::Interaction::response()
                        .with_content("Our friend " + un + " voted for " + option.name +
                                      " (Total: " + std::to_string(option.votes) + ")")
                        .as_json();
                }(p[msgid]);
                bot.send_json(resp, interaction);
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
