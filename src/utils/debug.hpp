#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <iostream>

namespace qb::log
{
template <typename T>
void normal(T t)
{
    std::cout << t << std::endl;
}

/** Logs plain, unformatted data. **/
template <typename T, typename... Ts>
void normal(T t, Ts... ts)
{
    std::cout << t;
    normal(std::forward<Ts>(ts)...);
}

template <typename T>
void func(T t)
{
    std::cout << t;
    std::flush(std::cout);
}

/** Logs plain, unformatted data. **/
template <typename T, typename... Ts>
void func(T t, Ts... ts)
{
    std::cout << t;
    func(std::forward<Ts>(ts)...);
}

/** Logs things inside of quotes. **/
template <typename T>
void quoted(T t)
{
    std::cout << '"' << t << '"' << std::endl;
}

template <typename T, typename... Ts>
void quoted(T t, Ts... ts)
{
    std::cout << '"' << t << "\" ";
    normal(std::forward<Ts>(ts)...);
}

/** Logs the contents of a range. **/
template <typename R>
void range(const R& r)
{
    for (auto&& c : r)
        std::cout << c << ", ";
    std::cout << std::endl;
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

/** Logs an error message. **/
template <typename... Ts>
void err(Ts... ts)
{
    _err("Error: ", std::forward<Ts>(ts)...);
}

/** Logs a warning message. **/
template <typename... Ts>
void warn(Ts... ts)
{
    normal("Warning: ", std::forward<Ts>(ts)...);
}

/** Logs a message for a specific line of code. **/
template <typename... Ts>
void point(Ts... ts)
{
    normal("> ", std::forward<Ts>(ts)...);
}

/** Logs printable data with a prepended label. **/
template <typename L, typename... Ts>
void data(const L& label, Ts... ts)
{
    normal(">> DATA: ", label);
    normal(std::forward<Ts>(ts)...);
    normal("===========");
}

/** For logging the value of a variable. **/
template <typename K, typename V>
void value(const K& name, const V& value)
{
    normal(">>> The value of ", name, " is ", value);
}

/** For only logging in the event of an exception. **/
template <typename S> // S is printable
class scope
{
public:
    scope() = default;
    scope(const S& s) : s_(s), should_log_(true)
    {
    }

    void operator+=(const S& s)
    {
        s_ += s;
        should_log_ = true;
    }

    void clear()
    {
        s_          = S{};
        should_log_ = false;
    }

    void log_contents()
    {
        if (should_log_)
        {
            qb::log::point(s_);
        }
    }

    ~scope()
    {
        if (should_log_)
        {
            qb::log::point(s_);
        }
    }

private:
    S s_{};
    bool should_log_{false};
};



} // namespace qb::log

#endif // DEBUG_HPP
