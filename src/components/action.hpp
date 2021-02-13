#ifndef QB_ACTION_HPP
#define QB_ACTION_HPP

#include <optional>
#include <string>
#include <functional>
#include <unordered_map>

// TODO This is now pulling in JSON
// This is not exactly ideal
#include "api/message.hpp"

namespace qb
{
class Bot;

class Result {
public:
    enum class Value {
        Ok,
    };

    Result(std::string err) : err_(std::move(err)) {}
    Result(::qb::Result::Value result) : val(result) {}

    std::optional<::qb::Result::Value> val{};
private:
    std::string err_;
};

using Actions = std::unordered_map<std::string, std::function<::qb::Result(std::string, api::Message, qb::Bot&)>>;
}

#endif // QB_ACTION_HPP
