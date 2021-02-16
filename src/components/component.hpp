#ifndef QB_COMPONENT_HPP
#define QB_COMPONENT_HPP

#include <utility>

#include "action.hpp"

namespace qb
{
class Bot;

class Component
{
public:
    virtual void register_actions(Actions<>& actions) = 0;

protected:
    // Component provides a number of helper functions for children.

    /**
     * Register a group of actions that all follow the same format.
     */
    template <typename T, typename... Ts>
    void register_all(T& actions, typename T::value_type val, Ts... vals) const
    {
        actions.emplace(std::move(val));
        register_all(actions, std::forward<Ts>(vals)...);
    }
    template <typename T>
    void register_all(T& actions, typename T::value_type val) const
    {
        actions.emplace(std::move(val));
    }

    nlohmann::json send_removable_message(Bot& bot, const std::string& message, const std::string& channel);
    nlohmann::json send_yn_message(Bot& bot, const std::string& message, const std::string& channel);

    template<typename Func, typename Class>
    ActionCallback bind_action(Func&& func, Class* subclass) {
        using namespace std::placeholders;
        return std::bind(func, subclass, _1, _2, _3);
    }

private:
    qb::Result add_delete_reaction(const std::string& message_id, const api::Message&, Bot& bot);
    qb::Result add_yn_reaction(const std::string& message_id, const api::Message& message, Bot& bot);
};

} // namespace qb

#endif // QB_COMPONENT_HPP
