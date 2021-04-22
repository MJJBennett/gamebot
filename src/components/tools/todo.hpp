#ifndef TODO_CLASS_HPP
#define TODO_CLASS_HPP

#include "../component.hpp"

#include <vector>

#include <boost/asio/steady_timer.hpp>

namespace qb
{
class TodoComponent : public Component
{
public:
    qb::Result todo_io(const std::string& cmd, const api::Message& msg, Bot& bot);
    void register_actions(Actions<>& actions) override;

    void dump_debug() const;

private:
    std::vector<std::string> todos;
};

} // namespace qb
#endif
