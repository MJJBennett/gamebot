#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <iostream>

namespace qb
{
template <typename T>
void print(T t)
{
    std::cout << t << std::endl;
}

template <typename T, typename... Ts>
void print(T t, Ts... ts)
{
    std::cout << t << ' ';
    print(std::forward<Ts>(ts)...);
}

template <typename T>
void _error(T t)
{
    std::cerr << t << std::endl;
}

template <typename T, typename... Ts>
void _error(T t, Ts... ts)
{
    std::cerr << t << ' ';
    _error(std::forward<Ts>(ts)...);
}

template <typename T>
void error(T t)
{
    // Only called when printing error with one argument
    std::cerr << "Error: " << t << std::endl;
}

template <typename T, typename... Ts>
void error(T t, Ts... ts)
{
    std::cerr << "Error: " << t << ' ';
    _error(std::forward<Ts>(ts)...);
}
} // namespace qb

#endif // DEBUG_HPP
