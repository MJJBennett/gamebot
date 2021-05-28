#ifndef COMMANDS_CLASS_HPP
#define COMMANDS_CLASS_HPP

#include "../component.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include <boost/asio/steady_timer.hpp>

namespace qb
{
struct Poll {
    struct Option {
        std::string name;
        std::string id;
        size_t votes{0};
    };
    std::unordered_set<std::string> voters;
    std::vector<Option> options;
};

class CommandsComponent : public Component
{
public:
    // NECESSARY IMPLEMENTATIONS
    void register_actions(Actions<>& actions) override;

public:
    // NORMAL FUNCTIONS
    qb::Result add_poll(const std::string& cmd, const api::Message& msg, Bot& bot);

    void dump_debug() const;
private:
    std::unordered_map<std::string, Poll> polls;
};

} // namespace qb
#endif // SPRINT_CLASS_HPP
