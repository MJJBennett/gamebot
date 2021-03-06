#ifndef QB_ACTION_HPP
#define QB_ACTION_HPP

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <type_traits>

// TODO This is now pulling in JSON
// This is not exactly ideal
#include "api/message.hpp"
#include "api/reaction.hpp"
namespace qb
{
class Bot;

class Result
{
public:
    enum class Value
    {
        Ok,
        PersistCallback,
    };
    static Result ok()
    {
        return Result(Value::Ok);
    }

    Result(std::string err) : err_(std::move(err))
    {
    }
    Result(::qb::Result::Value result) : val(result)
    {
    }

    std::optional<::qb::Result::Value> val{};

private:
    std::string err_;
};

// A basic action is just the predefined (string, datatype, bot) callback we're using
template <typename DataType = api::Message>
using BasicAction = std::conditional_t<std::is_same_v<DataType, api::Reaction>, 
std::function<::qb::Result(const std::string&, const DataType&, qb::Bot&, int)>,
std::function<::qb::Result(const std::string&, const DataType&, qb::Bot&)>>;

// Helper as ActionCallback was previously used under the assumption that callbacks
// would only be in response to messages. Other datatypes may include Reactions, etc
using ActionCallback = BasicAction<api::Message>;

template <typename DataType = api::Message>
using Actions = std::unordered_map<std::string, BasicAction<DataType>>;
template <typename DataType = api::Message>
using MultiActions = std::unordered_map<std::string, std::vector<BasicAction<DataType>>>;

// DEPRECATED
template <typename DataType = api::Message>
bool execute_callbacks(Bot& bot, const std::string& key, const nlohmann::json& json_data, MultiActions<DataType>& callbacks)
{
    if (callbacks.find(key) == callbacks.end()) return false;

    auto& l = callbacks[key];

    l.erase(std::remove_if(l.begin(), l.end(),
                           [&](auto& cb) {
                               auto res = cb(key, DataType::create(json_data), bot);
                               // TODO we support errors? apparently? probably no good
                               return *res.val != qb::Result::Value::PersistCallback;
                           }),
            l.end());

    if (l.empty()) callbacks.erase(key);

    return false; // return true when the action wants to not parse as a command, TODO
}
// DEPRECATED
template <typename DataType = api::Message>
bool execute_callbacks(Bot& bot, const std::string& key, const nlohmann::json& json_data, MultiActions<DataType>& callbacks, int choice)
{
    if (callbacks.find(key) == callbacks.end()) return false;

    auto& l = callbacks[key];

    l.erase(std::remove_if(l.begin(), l.end(),
                           [&](auto& cb) {
                               auto res = cb(key, DataType::create(json_data), bot, choice);
                               // TODO we support errors? apparently? probably no good
                               return *res.val != qb::Result::Value::PersistCallback;
                           }),
            l.end());

    if (l.empty()) callbacks.erase(key);

    return false; // return true when the action wants to not parse as a command, TODO
}

template <typename DataType = api::Message>
bool execute_callbacks(Bot& bot, const std::string& key, const DataType& data, MultiActions<DataType>& callbacks)
{
    if (callbacks.find(key) == callbacks.end()) return false;

    auto& l = callbacks[key];

    l.erase(std::remove_if(l.begin(), l.end(),
                           [&](auto& cb) {
                               auto res = cb(key, data, bot);
                               // TODO we support errors? apparently? probably no good
                               return *res.val != qb::Result::Value::PersistCallback;
                           }),
            l.end());

    if (l.empty()) callbacks.erase(key);

    return false; // return true when the action wants to not parse as a command, TODO
}

template <typename DataType = api::Message>
bool execute_callbacks(Bot& bot, const std::string& key, const DataType& data, MultiActions<DataType>& callbacks, int choice)
{
    if (callbacks.find(key) == callbacks.end()) return false;

    auto& l = callbacks[key];

    l.erase(std::remove_if(l.begin(), l.end(),
                           [&](auto& cb) {
                               auto res = cb(key, data, bot, choice);
                               // TODO we support errors? apparently? probably no good
                               return *res.val != qb::Result::Value::PersistCallback;
                           }),
            l.end());

    if (l.empty()) callbacks.erase(key);

    return false; // return true when the action wants to not parse as a command, TODO
}
} // namespace qb

#endif // QB_ACTION_HPP
