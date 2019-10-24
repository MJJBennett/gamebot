#ifndef FILE_IO
#define FILE_IO

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace qb::fileio
{
namespace skribbl
{
bool storage_exists();
nlohmann::json get_data();
}

void add_default(const std::vector<std::string>&);
void add_to_set(const std::string& set, const std::vector<std::string>&);

std::vector<std::string> get_all();
std::vector<std::string> get_set(const std::string& set);
std::vector<std::string> get_sets(const std::vector<std::string>& sets);
} // namespace qb::fileio

#endif // FILE_IO
