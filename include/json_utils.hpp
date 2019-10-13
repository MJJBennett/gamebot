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

inline std::string simple_prettify(const std::string& data)
{
    return json::parse(data).dump(2);
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