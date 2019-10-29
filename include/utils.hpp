#ifndef UTILS_HPP
#define UTILS_HPP
#include <string>


namespace qb::detail
{
std::string get_bot_token();
} // namespace qb::detail

namespace qb
{
// Performs a bitwise and (&) on the two parameters after
// static casting both of them to the template parameter.
template <typename T, typename L, typename R>
T bitwise_and(L l, R r)
{
    return (static_cast<T>(l) & static_cast<T>(r));
}

template <typename R>
bool range_eq(const R& r, const R& r2)
{
    assert(r.size() == r2.size());
    for (size_t it = 0; it < r.size(); it++)
    {
        if (r[it] != r2[it]) return false;
    }
    return true;
}

// Case-insensitive string compare
bool iequals(const std::string& a, const std::string& b);

} // namespace qb

#endif // UTILS_HPP
