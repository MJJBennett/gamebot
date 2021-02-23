#include "fileio.hpp"

#include "components/config.hpp"
#include "debug.hpp"
#include "json_utils.hpp"
#include "parse.hpp"
#include <filesystem>
#include <fstream>
#include <unordered_map>

struct emote_cache;
void invalidate_emote_cache();
emote_cache& get_emote_cache();

bool qb::fileio::skribbl::storage_exists()
{
    return std::filesystem::exists(qb::config::skribbl_data_file());
}

std::vector<std::string> qb::fileio::skribbl::keys()
{
    const auto d = get_data();
    std::vector<std::string> keylist;
    for (auto& [key, value] : d.items())
        keylist.push_back(key);
    return keylist;
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

    if (j.find(set) == j.end()) return {"null"};

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

void qb::fileio::register_emote(std::string name, std::string emote)
{
    // Open and store in our very simple Key/Value store
    std::ofstream os(qb::config::emote_data_file(), std::ios_base::app | std::ios_base::out);

    os << qb::parse::trim(name) << "=" << qb::parse::trim(emote) << '\n';
    invalidate_emote_cache();
}

struct emote_cache
{
    bool valid{false};
    std::string at(const std::string lookup)
    {
        if (!valid) reload();
        if (auto it = emote_map.find(lookup); it != emote_map.end()) return it->second;
        return qb::config::default_emote();
    }

    [[nodiscard]] bool contains(const std::string& lookup)
    {
        if (!valid) reload();
        return emote_map.find(lookup) != emote_map.end();
    }

    void reload()
    {
        std::ifstream is(qb::config::emote_data_file());
        if (!is)
        {
            // Couldn't open, assume the file doesn't exist yet.
            qb::log::warn("Could not open emote data file.");
            return;
        }
        std::string emote_mapping;
        size_t line = 0;
        while (getline(is, emote_mapping))
        {
            line++;
            auto it = std::find(emote_mapping.begin(), emote_mapping.end(), '=');
            if (it == emote_mapping.end() || it + 1 == emote_mapping.end())
            {
                qb::log::warn("Invalid emote mapping found while reloading at line ", line, ": ", emote_mapping);
                continue;
            }
            auto name  = std::string{emote_mapping.begin(), it};
            auto emote = std::string{it + 1, emote_mapping.end()};
            if (emote_map.find(name) != emote_map.end())
            {
                emote_map[name] = std::move(emote);
            }
            else
                emote_map.emplace(std::move(name), std::move(emote));
        }
    }

private:
    std::unordered_map<std::string, std::string> emote_map;
};

std::string qb::fileio::get_emote(std::string name)
{
    auto& cache = get_emote_cache();
    if (!cache.contains(name)) return "none";
    return cache.at(name);
}

std::vector<std::string> qb::fileio::get_emotes(const std::vector<std::string>& names)
{
    std::vector<std::string> res;
    for (const auto& name : names)
    {
        // For now. This is bad. In the future, we should REALLY
        // just return the emotes, and not this.
        res.emplace_back(name + ": " + get_emote(name));
    }
    return res;
}

emote_cache& get_emote_cache()
{
    static emote_cache ec;
    return ec;
}

void invalidate_emote_cache()
{
    get_emote_cache().valid = false;
}
