#ifndef QB_COMPONENT_HPP
#define QB_COMPONENT_HPP

#include <utility>

#include "action.hpp"

namespace qb
{
class Component
{
public:
    virtual void register_actions(Actions& actions) = 0;

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
};

} // namespace qb

#endif // QB_COMPONENT_HPP
