#ifndef GAME_QUEUE_CLASS_HPP
#define GAME_QUEUE_CLASS_HPP

#include "../component.hpp"

namespace qb
{
class Queue {
    public:
    Queue(const std::string name, api::Message message, const std::string game, int max_size) 
        : name_(name), message_(message), game_(game), max_size_(max_size) {}
    

    std::string to_str();
    std::string name_;
    api::Message message_;
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
    nlohmann::json send_yn_message(Queue& queue, Bot& bot, const std::string& message, const std::string& channel);
    void register_actions(Actions<>& actions) override;
    
private:
    std::unordered_map<std::string, Queue> active_queues;

    qb::Result add_yn_reaction( const std::string& message_id, const api::Message& message, Bot& bot);
    
};



}
#endif