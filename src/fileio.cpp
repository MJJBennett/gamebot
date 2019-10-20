#include "fileio.hpp"

#include "config.hpp"
#include "json_utils.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

void qb::fileio::add_default(const std::vector<std::string>& words)
{
    nlohmann::json j;
    if (std::filesystem::exists(qb::config::skribbl_data_file()))
    {
        std::ifstream i(qb::config::skribbl_data_file());
        i >> j;
    }

    if (!qb::json_utils::in(j, "default"))
    {
        j["default"] = std::vector<std::string>{};
    }

    for (const auto& w : words)
    {
        j["default"].push_back(w);
    }

    // Write the new data out again
    std::ofstream o(qb::config::skribbl_data_file());
    o << j << std::endl;
}
