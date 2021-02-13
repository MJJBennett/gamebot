#ifndef QB_ACTION_HPP
#define QB_ACTION_HPP

#include <optional>
#include <string>
#include <functional>
#include <unordered_map>

namespace qb
{
class Bot;

class Result {
public:
    enum class Value {

    };

    Result(std::string err) : err_(std::move(err)) {}
    Result(::qb::Result::Value result) : val(result) {}

    std::optional<::qb::Result::Value> val{};
private:
    std::string err_;
};

using Actions = std::unordered_map<std::string, std::function<::qb::Result(/*std::vector<*/std::string/*>*/, qb::Bot&)>>;
}

#endif // QB_ACTION_HPP
