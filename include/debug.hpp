#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <iostream>

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

#endif  // DEBUG_HPP
