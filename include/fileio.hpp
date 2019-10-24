#ifndef FILE_IO
#define FILE_IO

#include <string>
#include <vector>

namespace qb::fileio
{
namespace skribbl
{
bool storage_exists();
}

void add_default(const std::vector<std::string>&);

std::vector<std::string> get_all();
} // namespace qb::fileio

#endif // FILE_IO
