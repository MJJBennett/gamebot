#include "utils.hpp"

#include <algorithm>

std::string qb::detail::get_bot_token()
{
#include "client_tok.hpp"
    // Compile-time file I/O.
    // TODO - Don't do this.
    return __client_tok;
}

bool qb::iequals(const std::string& a, const std::string& b)
{
    return std::equal(a.cbegin(), a.cend(), b.cbegin(), b.cend(),
                      [](char l, char r) { return tolower(l) == tolower(r); });
}
