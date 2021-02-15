#ifndef JSON_UTILS_HPP
#define JSON_UTILS_HPP

#include <nlohmann/json.hpp>

namespace qb::json_utils
{
using json = nlohmann::json;

template <typename V>
bool val_eq(const json& jd, const std::string& key, const V& v)
{
    if (const auto val_it = jd.find(key); val_it != jd.end())
    {
        return (*val_it).get<V>() == v;
    }
    return false;
}

template <typename D>
D def(const json& jd, const std::string& key, const D& d)
{
    if (const auto val_it = jd.find(key); val_it != jd.end())
    {
        return (*val_it).get<D>();
    }
    return d;
}

constexpr auto def_str = def<std::string>;

template <typename T>
std::optional<T> get_opt(const json& jd, const std::string& key)
{
    if (const auto val_it = jd.find(key); val_it != jd.end())
    {
        return (*val_it).get<T>();
    }
    return {};
}

inline bool in(const json& jd, const std::string& key)
{
    return jd.find(key) != jd.end();
}

template <typename T>
bool has_path(const T& jd, const std::string& p)
{
    return in(jd, p);
}

template <typename T, typename... Ts>
bool has_path(const T& jd, const std::string& p, const Ts&... ts)
{
    if (!in(jd, p)) return false;
    return has_path(jd[p], std::forward<const Ts&>(ts)...);
}

inline json get_identify_packet(const std::string& token)
{
    return json{{"op", 2},
                {"d",
                 {{"token", token},
                  {"properties",
                   {{"$os", "macOS"},
                    {"$browser", "QueueBot"},
                    {"$device", "QueueBot"},
                    {"$referrer", ""},
                    {"$referring_domain", ""}}}}}};
}

} // namespace qb::json_utils

#endif // JSON_UTILS_HPP
