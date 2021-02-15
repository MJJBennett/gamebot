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
    static Result ok() { return Result(Value::Ok); }

    Result(std::string err) : err_(std::move(err)) {}
    Result(::qb::Result::Value result) : val(result) {}

    std::optional<::qb::Result::Value> val{};
private:
    std::string err_;
};

using ActionCallback = std::function<::qb::Result(const std::string&, const api::Message&, qb::Bot&)>;

using Actions = std::unordered_map<std::string, ActionCallback>;
using MultiActions = std::unordered_map<std::string, std::vector<ActionCallback>>;
}

#endif // QB_ACTION_HPP
