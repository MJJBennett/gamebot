#ifndef COMMANDS_CLASS_HPP
#define COMMANDS_CLASS_HPP

#include "../component.hpp"

#include <chrono>
#include <memory>
#include <optional>

#include <boost/asio/steady_timer.hpp>

namespace qb
{
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
};

} // namespace qb
#endif // SPRINT_CLASS_HPP
