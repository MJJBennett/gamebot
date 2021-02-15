#include "gtest/gtest.h"

#include "components/component.hpp"
#include "components/action.hpp"

#include <functional>
#include <string>
#include <utility>

using namespace qb;

class TestComponent : public Component
{
public:
    qb::Result action_one(const std::string& cmd, const api::Message& msg, Bot& bot) {
        return qb::Result::ok();
    }
    qb::Result action_two(const std::string& cmd, const api::Message& msg, Bot& bot){
        return qb::Result::ok();}

    void register_actions(Actions<>& actions) override
    {
        using namespace std::placeholders;
        register_all(
            actions,
            std::make_pair("one", (ActionCallback)std::bind(&TestComponent::action_one, this, _1, _2, _3)),
            std::make_pair("two", (ActionCallback)std::bind(&TestComponent::action_two, this, _1, _2, _3)));
    }
};

TEST(Component, Register)
{
    TestComponent test;

    ::qb::Actions<> actions;

    test.register_actions(actions);

    ASSERT_EQ(actions.size(), 2);
}

TEST(Component, Callback)
{
    TestComponent test;

    ::qb::Actions<> actions;

    test.register_actions(actions);

    const auto itr1 = actions.find("one");
    ASSERT_NE(itr1, actions.end());
    const auto itr2 = actions.find("two");
    ASSERT_NE(itr2, actions.end());
    const auto itr3 = actions.find("three");
    ASSERT_EQ(itr3, actions.end());

    int _cursed = 0;
    const auto _unused = actions["one"]("", api::Message::create<true>(nlohmann::json{}), *(Bot*)(&_cursed));
}
