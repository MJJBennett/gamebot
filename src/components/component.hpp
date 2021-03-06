#ifndef QB_COMPONENT_HPP
#define QB_COMPONENT_HPP

#include <utility>

#include "action.hpp"
#include "button.hpp"

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

    nlohmann::json send_removable_message(Bot& bot, const std::string& message, const api::Channel& channel);

    nlohmann::json send_buttons(Bot& bot,
                                const std::string& message,
                                const std::vector<qb::Button>& buttons,
                                const api::Channel& channel);

    template <typename Func, typename Class>
    ActionCallback bind_message_action(Func&& func, Class* subclass)
    {
        using namespace std::placeholders;
        return std::bind(func, subclass, _1, _2, _3);
    }
    template <typename Func, typename Class>
    ActionCallback bind_reaction_action(Func&& func, Class* subclass)
    {
        using namespace std::placeholders;
        return std::bind(func, subclass, _1, _2, _3, _4);
    }

private:
    qb::Result add_delete_reaction(const std::string& message_id, const api::Message&, Bot& bot);
};

} // namespace qb

#endif // QB_COMPONENT_HPP
