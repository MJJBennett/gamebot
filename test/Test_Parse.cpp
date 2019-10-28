#include "parse.hpp"

#include "gtest/gtest.h"
#include <string>

using namespace qb::parse;

TEST(Parse, remove_non_cmd)
{
    std::string a{"!qb do something"};
    std::string b{"<@222222222> !qb do otherwise!"};
    EXPECT_EQ(remove_non_cmd(a), "!qb do something");
    EXPECT_EQ(remove_non_cmd(b), "!qb do otherwise!");
}

TEST(Parse, split)
{
    std::string a{"this is a simple string"};
    std::string b{" this    is a   second sample       string      "};

    std::vector<std::string> a_ans{"this", "is", "a", "simple", "string"};
    std::vector<std::string> b_ans{"this", "is", "a", "second", "sample", "string"};

    auto vts = [](std::vector<std::string> s) {
        std::string ret{};
        for (auto&& str : s)
        {
            ret += str;
        }
        return ret;
    };

    EXPECT_EQ(vts(split(a)), vts(a_ans));
    EXPECT_EQ(vts(split(b)), vts(b_ans));
}

TEST(Parse, GetCommandName)
{
    std::string a{"command:name:with:specifiers and some arguments"};

    EXPECT_EQ(get_command_name(a), "command:name:with:specifiers");
}

TEST(Parse, split_2)
{
    std::string a{"command:name:with:specifiers and some arguments"};

    std::vector<std::string> a_ans{"command", "name", "with", "specifiers"};

    EXPECT_EQ(concatenate(split(get_command_name(a), ':')), concatenate(a_ans));
}

TEST(Parse, get_time)
{
    std::vector<std::string> set_1{"0m4s5h3s500s333h", "something", "something else"};
    auto [time1, others1] = get_time(set_1);

    EXPECT_EQ(time1, "0m4s5h3s500s333h");
    EXPECT_EQ(others1.size(), 2);

    std::vector<std::string> set_2{"something", "5m", "something else"};
    auto [time2, others2] = get_time(set_2);

    EXPECT_EQ(time2, "5m");
    EXPECT_EQ(others2.size(), 2);
}
