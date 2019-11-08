#include "sentiment.hpp"

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
    std::vector<int> sentiments(words.size());
    std::transform(words.cbegin(), words.cend(), sentiments.begin(),
                   [](const std::string& s) { return value(s); });
    auto total_sentiment = std::accumulate(sentiments.cbegin(), sentiments.cend(), 0);
    if (total_sentiment < default_negative_threshold(words.size())) return true;

    // const auto negative_words =
    //    std::transform_reduce(words.begin(), words.end(), static_cast<int>(0), std::plus<>{},
    //                          [](std::string word) { return static_cast<int>(word.size()); });
    return false;
}

int qb::sentiment::value(const std::string& str)
{
    const static std::unordered_map<std::string, int> sentimental_words = {
        {key("bad"), -1}, {key("sad"), -1}, {key("hate"), -2}};

    if (auto it = sentimental_words.find(key(str)); it != sentimental_words.end())
    {
        return it->second;
    }

    return 0;
}

int qb::sentiment::default_negative_threshold(const size_t len)
{
    return 0 - (len * 0.3);
}

std::string qb::sentiment::key(const std::string& in)
{
    const static std::string vowels{"aeiouy"};
    std::string output;
    std::for_each(in.cbegin(), in.cend(), [&](char c) {
        if (qb::parse::in(c, vowels)) output += c;
    });
    return output;
}

