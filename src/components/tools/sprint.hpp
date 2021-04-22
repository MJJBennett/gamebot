#ifndef SPRINT_CLASS_HPP
#define SPRINT_CLASS_HPP

#include "../component.hpp"

#include <chrono>
#include <memory>
#include <optional>

#include <boost/asio/steady_timer.hpp>

namespace qb
{
class Sprint
{
public:
    Sprint(std::optional<std::string> name,
          std::chrono::duration<long> time)
        : name_(name), time_(time)
    {
    }

    std::string to_str() const;
    std::string get_name() const
    {
        return name_ ? *name_ : "Unnamed sprint.";
    }
    const std::optional<std::string> name_;

    // contains user_ids currently in sprint
    std::vector<std::string> users;

    std::chrono::duration<long> time_;

    std::optional<boost::asio::steady_timer> timer_{};
};

class SprintComponent : public Component
{
public:
    qb::Result add_sprint(const std::string& cmd, const api::Message& msg, Bot& bot);
    qb::Result remove_sprint(const std::string& cmd, const api::Message& msg, Bot& bot);
    nlohmann::json send_yn_message(Sprint&& sprint, Bot& bot, const std::string& message, const std::string& channel);
    void register_actions(Actions<>& actions) override;

    qb::Result end_sprint(const std::string& message_id, const api::Reaction& reaction, Bot& bot);
    void dump_debug() const;

private:
    std::unordered_map<std::string, Sprint> active_sprints;

    qb::Result add_yn_reaction(const std::string& message_id, const api::Message& message, Bot& bot);
};

} // namespace qb
#endif
