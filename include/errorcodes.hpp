#ifndef ERRORCODES_HPP
#define ERRORCODES_HPP

#include <string>
#include <string_view>

namespace qb
{
using std::string_view_literals::operator""sv;

constexpr auto oversend = ""sv;
constexpr auto unknown = ""sv;

std::string_view err_str(const int ec){
if (ec == 4002) return oversend;
return unknown;
}
}  // namespace qb::constants

#endif //ERRORCODES_HPP
