#include "todo.hpp"

#include "bot.hpp"
#include "utils/fileio.hpp"
#include "utils/parse.hpp"
#include "utils/utils.hpp"
#include "components/config.hpp"

#include <boost/asio/steady_timer.hpp>

void qb::TodoComponent::register_actions(Actions<>& actions)
{
    using namespace std::placeholders;
    register_all(
        actions,
        std::make_pair("todo", (ActionCallback)std::bind(&qb::TodoComponent::todo_io, this, _1, _2, _3)),
        std::make_pair("db:todo", bind_message_action(
                                      [](qb::TodoComponent* obj, const std::string&, const api::Message&, Bot&) {
                                          obj->dump_debug();
                                          return qb::Result::Value::Ok;
                                      },
                                      this)));
}

qb::Result qb::TodoComponent::todo_io(const std::string& cmd, const api::Message& msg, Bot& bot)
{
    // we should abstract this away sometime
    auto endpoint = msg.endpoint();
    const auto& channel = msg.channel;
    bot.get_context()->del(endpoint);

    // todo io:
    // [d]elete a todo
    // [a]dd a todo
    // [l]ist all todos
    // [r]eload todos

    if (qb::parse::startswith(cmd, "d"))
    {
    }
    else if (qb::parse::startswith(cmd, "a"))
    {
    }
    else if (qb::parse::startswith(cmd, "r"))
    {
        todos = qb::fileio::readlines_nonempty(qb::config::todo_data_file());
    }
    else
    {
        std::string to_send;
        if (todos.empty()) todos = qb::fileio::readlines_nonempty(qb::config::todo_data_file());
        if (todos.empty()) to_send = "There are no saved todos right now.";
        else for (size_t i = 0; i < todos.size(); i++) {
            to_send += std::to_string(i + 1) + ") ";
            to_send += todos[i];
            if (i < todos.size() - 1) to_send += "\n";
        }
        send_removable_message(bot, to_send, channel);
    }

    return qb::Result::ok();
}

void qb::TodoComponent::dump_debug() const
{
    qb::log::point("Dumping TodoComponent debug information. [START]");
    qb::log::point("TodoComponent contains ", todos.size(), " active todos.");
    qb::log::point("Finished dumping TodoComponent debug information. [END]");
}
