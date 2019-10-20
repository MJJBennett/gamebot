#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <nlohmann/json.hpp>
#include <string>

namespace qb::config
{
using json                                = nlohmann::json;
const auto conf_                          = json{{"command_start", "!qb "}};
constexpr int max_skribbl_wordsize        = 30;
const std::string max_skribble_wordsize_s = "30";

inline std::string cmd_start()
{
    return conf_["command_start"];
}

} // namespace qb::config

#endif // CONFIG_HPP
