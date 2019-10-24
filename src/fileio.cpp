#include "fileio.hpp"

#include "config.hpp"
#include "json_utils.hpp"
#include <filesystem>
#include <fstream>

bool qb::fileio::skribbl::storage_exists()
{
    return std::filesystem::exists(qb::config::skribbl_data_file());
}

nlohmann::json qb::fileio::skribbl::get_data()
{
    nlohmann::json j;
    if (skribbl::storage_exists())
    {
        std::ifstream i(qb::config::skribbl_data_file());
        i >> j;
    }
    return j;
}

void qb::fileio::add_default(const std::vector<std::string>& words)
{
    auto j = skribbl::get_data();

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

void qb::fileio::add_to_set(const std::string& set, const std::vector<std::string>& words)
{
    auto j = skribbl::get_data();

    if (!qb::json_utils::in(j, set))
    {
        j[set] = std::vector<std::string>{};
    }

    for (const auto& w : words)
    {
        j[set].push_back(w);
    }

    // Write the new data out again
    std::ofstream o(qb::config::skribbl_data_file());
    o << j << std::endl;
}

std::vector<std::string> qb::fileio::get_all()
{
    if (!skribbl::storage_exists()) return {};

    auto j = skribbl::get_data();
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

std::vector<std::string> qb::fileio::get_set(const std::string& set)
{
    if (!skribbl::storage_exists()) return {};

    auto j = skribbl::get_data();

    return j.at(set).get<std::vector<std::string>>();
}

std::vector<std::string> qb::fileio::get_sets(const std::vector<std::string>& sets)
{
    if (!skribbl::storage_exists()) return {};

    auto j = skribbl::get_data();
    std::vector<std::string> names;

    for (auto&& set : sets)
    {
        for (auto&& name : j.at(set).get<std::vector<std::string>>())
        {
            names.emplace_back(std::move(name));
        }
    }
    return names;
}
