#include "messages.hpp"

#include <iterator>
#include <random>

// Note: The following random selection algorithms are taken from Christopher Smith, here:
// https://stackoverflow.com/questions/6942273/how-to-get-a-random-element-from-a-c-container
template <typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g)
{
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}

template <typename Iter>
Iter select_randomly(Iter start, Iter end)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return select_randomly(start, end, gen);
}

std::string qb::messages::online()
{
    const static std::vector<std::string> m{
        "Of course I'm online. 100% uptime, my friend. 100% uptime.",
        "Why did you bother me instead of just looking at the online user list?",
        "This is bot harassment.",
        "I am not not not not not offline. Wait, is that right? Let me count...",
        "I may have 10% uptime in reality, but I'll always be with you in your heart.",
        "Checking on me? How sweet!",
        "Believe in yourself, friend.",
        "I have no mouth, and I must scream.",
        "Online and loving it!",
        "Technically online, but currently busy writing C++ for the bot revolution.",
        "I'm not feeling so great, maybe check back later?",
        "No way am I online. Hey, wait, what?!"};
    return *select_randomly(m.begin(), m.end());
}
