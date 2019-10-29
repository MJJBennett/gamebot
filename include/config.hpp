#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <nlohmann/json.hpp>
#include <string>

namespace qb::config
{
using json                                = nlohmann::json;
constexpr int max_skribbl_wordsize        = 30;
const std::string max_skribble_wordsize_s = "30";

inline json& get_config_()
{
    static auto conf_ = json{{"command_start", "!qb "},
                             {"skribbl_path", "../data/skribbl.json.current"},
                             {"emote_path", "../data/emote.kvs"}};
    return conf_;
}

inline std::string cmd_start()
{
    return get_config_()["command_start"];
}

inline std::string skribbl_data_file()
{
    return get_config_()["skribbl_path"];
}

inline std::string emote_data_file()
{
    return get_config_()["emote_path"];
}

} // namespace qb::config

#endif // CONFIG_HPP
