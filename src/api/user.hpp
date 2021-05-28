#ifndef QB_USER_HPP
#define QB_USER_HPP

#include <nlohmann/json.hpp>
#include <optional>

#include "utils/json_utils.hpp"

namespace qb::api
{
struct User
{
public:
    User() = default;
    User(const std::string& id, const std::string& username) : id(id), username(username)
    {
    }
    User(const std::string& id) : id(id){}

    User(const User& other) : id(other.id), username(other.username) {}

    template <bool safe = false>
    static User create(const nlohmann::json& source)
    {
        using namespace qb::json_utils;
        if constexpr (safe)
        {
            if (!in(source, "id") || !in(source, "username"))
            {
                ::qb::log::err("User being constructed from incorrect tier ",
                               "of JSON data (or invalid data):\n", source.dump(2));
                return User{};
            }
        }

        return User(source["id"], source["username"]);
    }

    const std::string id{};
    const std::optional<std::string> username{};
};
} // namespace qb::api

#endif // QB_USER_HPP
