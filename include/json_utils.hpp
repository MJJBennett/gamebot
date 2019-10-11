#ifndef JSON_UTILS_HPP
#define JSON_UTILS_HPP

#include <nlohmann/json.hpp>
namespace qb::json
{
    using json = nlohmann::json;
    template<typename V>
bool val_eq (const json& jd, const std::string& key, const V& v){
if (const auto val_it = jd.find(key); val_it != jd.end())
{
    return (*val_it).get<V>() == v;
}
return false;
}
}

#endif // JSON_UTILS_HPP
