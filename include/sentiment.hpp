#ifndef SENTIMENT_ANALYSIS_HPP
#define SENTIMENT_ANALYSIS_HPP

#include <string>

namespace qb
{
namespace sentiment
{
bool is_negative(const std::string&);

float value(const std::string&);

float default_negative_threshold(const size_t len);

std::string key(const std::string& in);
}

} // namespace qb

#endif // SENTIMENT_ANALYSIS_HPP
