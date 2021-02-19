#ifndef GAME_QUEUE_CLASS_HPP
#define GAME_QUEUE_CLASS_HPP

#include "../component.hpp"

#include <optional>

namespace qb
{
class Queue
{
public:
    Queue(std::optional<std::string> name, std::optional<std::string> game, std::optional<int> max_size, std::optional<std::chrono::duration<long>> time)
        : name_(name), game_(game), max_size_(max_size), time_(time)
    {
    }

    std::string to_str() const;
    std::string get_name() const
    {
        return name_ ? *name_ : "anonymous name";
    }
    std::string get_game() const
    {
        return game_ ? *game_ : "anonymous game";
    }
    const std::optional<std::string> name_;
    const std::optional<std::string> game_;

    // contains user_ids currently in queue
    std::vector<std::string> users;

    // maximum number of users allowed in queue
    std::optional<int>  max_size_;

    std::optional<std::chrono::duration<long>> time_;
};

class QueueComponent : public Component
{
public:
    qb::Result add_queue(const std::string& cmd, const api::Message& msg, Bot& bot);
    qb::Result remove_queue(const std::string& cmd, const api::Message& msg, Bot& bot);
    nlohmann::json send_yn_message(Queue& queue, Bot& bot, const std::string& message, const std::string& channel);
    void register_actions(Actions<>& actions) override;

private:
    std::unordered_map<std::string, Queue> active_queues;

    qb::Result add_yn_reaction(const std::string& message_id, const api::Message& message, Bot& bot);
};

} // namespace qb
#endif
