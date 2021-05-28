#ifndef QB_BUTTON_HPP
#define QB_BUTTON_HPP

#include <nlohmann/json.hpp>
#include <string>

namespace qb
{
struct Button
{
    std::string to_string() const
    {
        return to_json().dump();
    }
    nlohmann::json to_json() const
    {
        nlohmann::json my_component{
            {"type", type}, {"label", label}, {"style", style}, {"custom_id", custom_id}};
        return my_component;
    }

    Button(std::string custom_id, std::string label) : custom_id(custom_id), label(label) {}

    // Button components have type of 2
    static constexpr size_t type = 2;

    size_t style{1};
    std::string custom_id;
    std::string label;
};
} // namespace qb

#endif // QB_BUTTON_HPP
