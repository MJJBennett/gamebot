#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <nlohmann/json.hpp>
#include <string>

namespace qb::config
{
using json       = nlohmann::json;
const auto conf_ = json{{"command_start", "!qb "}};

inline std::string cmd_start()
{
    return conf_["command_start"];
}

} // namespace qb::config

#endif // CONFIG_HPP
