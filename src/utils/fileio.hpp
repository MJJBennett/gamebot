#ifndef FILE_IO
#define FILE_IO

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace qb::fileio
{
namespace skribbl
{
bool storage_exists();
nlohmann::json get_data();
std::vector<std::string> keys();
} // namespace skribbl

void add_default(const std::vector<std::string>&);
void add_to_set(const std::string& set, const std::vector<std::string>&);

std::vector<std::string> get_all();
std::vector<std::string> get_set(const std::string& set);
std::vector<std::string> get_sets(const std::vector<std::string>& sets);

void register_emote(std::string name, std::string emote);
std::string get_emote(std::string name);
std::vector<std::string> get_emotes(const std::vector<std::string>& names);
} // namespace qb::fileio

#endif // FILE_IO
