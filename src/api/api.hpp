#ifndef QB_API_HPP
#define QB_API_HPP

#include <nlohmann/json.hpp>

namespace qb::api
{
inline bool is_webhook(const nlohmann::json& source)
{
    return source.find("webhook_id") != source.end();
}
} // namespace qb::api

#endif // QB_API_HPP
