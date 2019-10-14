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
    std::cout << t;
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
    std::cerr << t;
    _err(std::forward<Ts>(ts)...);
}

template <typename... Ts>
void err(Ts... ts)
{
    _err("Error: ", std::forward<Ts>(ts)...);
}

template <typename... Ts>
void warn(Ts... ts)
{
    normal("Warning: ", std::forward<Ts>(ts)...);
}

template <typename... Ts>
void point(Ts... ts)
{
    normal("> ", std::forward<Ts>(ts)...);
}

template <typename L, typename... Ts>
void data(const L& label, Ts... ts)
{
    normal(">> DATA: ", label);
    normal(std::forward<Ts>(ts)...);
    normal("===========");
}

template <typename K, typename V>
void value(const K& name, const V& value)
{
    normal(">>> The value of ", name, " is ", value);
}
} // namespace log
} // namespace qb

#endif // DEBUG_HPP
