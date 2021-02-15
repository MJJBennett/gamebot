#ifndef QB_EMOJI_HPP
#define QB_EMOJI_HPP

#include <nlohmann/json.hpp>
#include <optional>

#include "utils/json_utils.hpp"

namespace qb::api
{
struct Emoji
{
    Emoji() = default;
    Emoji(const std::string& id, const std::string& name) : id(id), name(name)
    {
    }
    Emoji(const std::string& id) : id(id){}

    std::string to_string() const {
        return "ID: " + (id ? *id : std::string{"null"}) + "; Name: " + (name ? *name : std::string{"null"}) + ";";
    }

    template <bool safe = false>
    static Emoji create(const nlohmann::json& source)
    {
        using namespace qb::json_utils;
        if constexpr (safe)
        {
            if (!in(source, "id") || !in(source, "name"))
            {
                ::qb::log::err("Emoji (maybe) being constructed from incorrect tier ",
                               "of JSON data (or invalid data):\n", source.dump(2));
                return Emoji{};
            }
        }
        if (source["name"].is_null()) {
            // This is why we have optional.
            return Emoji(source["id"], {});
        }
       
        if (source["id"].is_null()) {
            // This is why we have optional.
            return Emoji({}, source["name"]);
        }

        return Emoji(source["id"], source["name"]);
    }

    const std::optional<std::string> id{};
    const std::optional<std::string> name{};
};
} // namespace qb::api

#endif // QB_EMOJI_HPP
