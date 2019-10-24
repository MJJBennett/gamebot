#include "fileio.hpp"

#include "config.hpp"
#include "json_utils.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

bool qb::fileio::skribbl::storage_exists()
{
    return std::filesystem::exists(qb::config::skribbl_data_file());
}

void qb::fileio::add_default(const std::vector<std::string>& words)
{
    nlohmann::json j;
    if (skribbl::storage_exists())
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

std::vector<std::string> qb::fileio::get_all()
{
    if (!skribbl::storage_exists()) return {};

    nlohmann::json j;
    std::ifstream i(qb::config::skribbl_data_file());
    i >> j;
    std::vector<std::string> all_names;

    for (auto& [key, value] : j.items())
    {
        for (auto&& name : value)
        {
            all_names.emplace_back(std::move(name));
        }
    }
    return all_names;
}
