#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <iostream>

namespace qb
{
namespace log
{
template <typename T>
void normal(T t)
{
    std::cout << t << std::endl;
}

template <typename T, typename... Ts>
void normal(T t, Ts... ts)
{
    std::cout << t << ' ';
    normal(std::forward<Ts>(ts)...);
}

template <typename T>
void _err(T t)
{
    std::cerr << t << std::endl;
}

template <typename T, typename... Ts>
void _err(T t, Ts... ts)
{
    std::cerr << t << ' ';
    _err(std::forward<Ts>(ts)...);
}

template<typename... Ts>
void err(Ts... ts)
{
    _err("Error:", std::forward<Ts>(ts)...);
}
} // namespace log
} // namespace qb

#endif // DEBUG_HPP
