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
} // namespace qb

#endif // UTILS_HPP
