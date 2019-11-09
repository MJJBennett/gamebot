#include "sentiment.hpp"

#include "debug.hpp"
#include "parse.hpp"
#include <algorithm>
#include <numeric>
#include <unordered_map>

bool qb::sentiment::is_negative(const std::string& str)
{
    // We make no guarantees about the result of this function
    // The caller must take into account efficiency/memory usage

    const auto words = qb::parse::split(str);
    if (words.size() == 0) return false;
    std::vector<float> sentiments(words.size());
    std::transform(words.cbegin(), words.cend(), sentiments.begin(),
                   [](const std::string& s) { return value(s); });
    auto total_sentiment = std::accumulate(sentiments.cbegin(), sentiments.cend(), 0.0f);
    qb::log::point("Total sentiment: ", total_sentiment);
    if (total_sentiment < default_negative_threshold(words.size())) return true;

    // Apparently transform_reduce isn't properly implemented in GCC 9.2 - oh well.
    // const auto negative_words =
    //    std::transform_reduce(words.begin(), words.end(), static_cast<int>(0), std::plus<>{},
    //                          [](std::string word) { return static_cast<int>(word.size()); });
    return false;
}

float qb::sentiment::value(const std::string& str)
{
    static std::unordered_map<std::string, float> sentimental_words = {
        {key("bad"), -0.5}, {key("sad"), -0.5}, {key("hate"), -2}, {key("java"), -0.5}, {key("report"), -1.5}};

    if (auto it = sentimental_words.find(key(str)); it != sentimental_words.end())
    {
        qb::log::point("Value of word: ", str, ": ", it->second);
        return it->second;
    }

    return 0;
}

float qb::sentiment::default_negative_threshold(const size_t len)
{
    //    return 0 - (len * 0.3);
    // This is the threshold value that a series of words of length len
    // must be less than in order for a message to have a negative sentiment.
    // Algorithm thoughts:
    //  - The above idea is pretty simple. A message of length, say, 3 would
    //    theoretically have -1 as the threshold.
    //  - However, this is perhaps too strict. Maybe if half the words are
    //    effectively negative?
    qb::log::point("DNT for sz ", len, ": ", -1.0f - (static_cast<float>(len) * 0.1));
    return -1.0f - (static_cast<float>(len) * 0.1);
}

std::string qb::sentiment::key(const std::string& in)
{
    const static std::string vowels{"aeiouy"};
    const static std::string consonants{"bcdfghjklmnpqrstvwxz"};
    std::string output;
    //    for (size_t i =
    std::for_each(in.cbegin(), in.cend(), [&](char c) {
        c = tolower(c);
        if (qb::parse::in(c, consonants)) output += c;
    });
    return output;
}

