#ifndef GAME_QUEUE_CLASS_HPP
#define GAME_QUEUE_CLASS_HPP

#include "../component.hpp"

namespace qb
{
class Queue {
    public:
    Queue(std::string name, std::string guild_id, std::string channel, std::string game, int max_size) 
        : name_(name), guild_id_(guild_id), channel_(channel), game_(game), max_size_(max_size) {}
    
    private:
    
    std::string name_;
    std::string guild_id_;
    std::string channel_;
    std::string game_;

    //contains user_ids currently in queue
    std::vector<std::string> users;

    //maximum number of users allowed in queue
    const int max_size_; 
};

class QueueComponent : public Component
{
public:
    qb::Result add_queue(const std::string& cmd, const api::Message& msg, Bot& bot);
    qb::Result remove_queue(const std::string& cmd, const api::Message& msg, Bot& bot);
    nlohmann::json send_yn_message(Bot& bot, const std::string& name, const std::string& message, const std::string& channel);
    void register_actions(Actions<>& actions) override;
    
private:
    std::unordered_map<std::string, Queue> active_queues;

    qb::Result add_yn_reaction(const std::string& name, const std::string& message_id, const api::Message& message, Bot& bot);
    
};



}
#endif