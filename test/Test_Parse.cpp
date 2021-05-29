#include "utils/parse.hpp"

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

TEST(Parse, xsv)
{
    std::string a{"this,is,a,simple string"};
    std::string b{",send-help,\"Haha, good luck with that!\""};
    std::string c{",send-help,\"Haha, good luck with that!\",\"Here's another one!\""};

    std::vector<std::string> a_ans{"this", "is", "a", "simple string"};
    std::vector<std::string> b_ans{"", "send-help", "Haha, good luck with that!"};
    std::vector<std::string> c_ans{"", "send-help", "Haha, good luck with that!",
                                   "Here's another one!"};

    auto vts = [](std::vector<std::string> s) {
        std::string ret{};
        for (auto&& str : s)
        {
            ret += str;
        }
        return ret;
    };

    EXPECT_EQ(vts(xsv(a)), vts(a_ans));
    EXPECT_EQ(vts(xsv(b)), vts(b_ans));
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

TEST(Parse, emote_snowflake)
{
    const auto res1 = emote_snowflake("<:something:32511>");
    ASSERT_TRUE(res1);
    EXPECT_EQ(*res1, "something:32511");
    EXPECT_NE(*res1, "something::32511");

    /*
    const auto res2 = emote_snowflake("32511");
    ASSERT_TRUE(res2);
    EXPECT_EQ(*res2, "32511");
    */

    const auto res3 = emote_snowflake("none");
    ASSERT_FALSE(res3);
}

TEST(Parse, get_trailingest_digits)
{
    ASSERT_EQ(get_trailingest_digits("hello123hello"), std::string{"123"});
    ASSERT_EQ(get_trailingest_digits("hellohello"), std::string{""});
    ASSERT_EQ(get_trailingest_digits("he33lo123hello"), std::string{"123"});
}

TEST(Parse, is_valid_time_trailer)
{
    std::string s{"d5h"};
    std::string nd{s.begin(), std::find_if(s.begin(), s.end(), [](char c) { return isdigit(c); })};
    ASSERT_EQ(nd, "d");
    ASSERT_TRUE(::qb::parse::in('d', std::string{"hmsd"}));
    ASSERT_TRUE(is_valid_time_trailer("d5h"));
}

TEST(Parse, split_number)
{
    auto [fn, ft] = split_number("4d5h");
    ASSERT_TRUE(fn);
    ASSERT_FALSE(ft.empty());
    ASSERT_EQ(*fn, 4);
    ASSERT_EQ(ft, "d5h");

    auto [nn, nt] = split_number("5h");
    ASSERT_TRUE(nn);
    ASSERT_FALSE(nt.empty());
    ASSERT_EQ(*nn, 5);
    ASSERT_EQ(nt, "h");
}

TEST(Parse, decompose_argument)
{
    DecomposedCommand cmd;
    std::string s;

    s = "hello!";
    decompose_argument(cmd, s);
    ASSERT_EQ(cmd.arguments.size(), 1);
    ASSERT_EQ(cmd.arguments[0], "hello!");

    s = "4d5lifetime-subscriptions-to-walmart!";
    decompose_argument(cmd, s);
    EXPECT_EQ(cmd.numeric_arguments.size(), 0);
    EXPECT_EQ(cmd.duration_arguments.size(), 0);
    EXPECT_EQ(cmd.durations.size(), 0);
    ASSERT_EQ(cmd.arguments.size(), 2);
    ASSERT_EQ(cmd.arguments[1], "4d5lifetime-subscriptions-to-walmart!");

    s = "4d5h";
    decompose_argument(cmd, s);
    EXPECT_EQ(cmd.numeric_arguments.size(), 0);
    EXPECT_EQ(cmd.duration_arguments.size(), 1);
    EXPECT_EQ(cmd.durations.size(), 1);
    ASSERT_EQ(cmd.arguments.size(), 2);
    ASSERT_EQ(cmd.duration_arguments[0].count(),
              std::chrono::duration<long>((4 * 24 * 60 * 60) + 5 * 60 * 60).count());

    s = "4d5h5m2s";
    decompose_argument(cmd, s);
    EXPECT_EQ(cmd.numeric_arguments.size(), 0);
    EXPECT_EQ(cmd.duration_arguments.size(), 2);
    EXPECT_EQ(cmd.durations.size(), 2);
    ASSERT_EQ(cmd.arguments.size(), 2);
    ASSERT_EQ(cmd.duration_arguments[0].count(),
              std::chrono::duration<long>((4 * 24 * 60 * 60) + 5 * 60 * 60).count());
    ASSERT_EQ(cmd.duration_arguments[1].count(),
              std::chrono::duration<long>((4 * 24 * 60 * 60) + 5 * 60 * 60 + 5 * 60 + 2).count());
    ASSERT_EQ(cmd.durations[0], "4d5h");
    ASSERT_EQ(cmd.durations[1], "4d5h5m2s");

    s = "3421";
    decompose_argument(cmd, s);
    EXPECT_EQ(cmd.numeric_arguments.size(), 1);
    EXPECT_EQ(cmd.duration_arguments.size(), 2);
    ASSERT_EQ(cmd.arguments.size(), 2);
    ASSERT_EQ(cmd.numeric_arguments[0], 3421);
}

TEST(Parse, decompose_command)
{
    auto cmd =
        decompose_command(std::string{"empty-thing "
                                      "hello! "
                                      "4d5lifetime-subscriptions-to-walmart! "
                                      "4d5h "
                                      "4d5h5m2s "
                                      "3421"});

    ASSERT_EQ(cmd.numeric_arguments.size(), 1);
    ASSERT_EQ(cmd.arguments.size(), 2);
    ASSERT_EQ(cmd.duration_arguments.size(), 2);

    // Arguments
    ASSERT_EQ(cmd.arguments[0], "hello!");
    ASSERT_EQ(cmd.arguments[1], "4d5lifetime-subscriptions-to-walmart!");

    // Duration arguments
    ASSERT_EQ(cmd.duration_arguments[0].count(),
              std::chrono::duration<long>((4 * 24 * 60 * 60) + 5 * 60 * 60).count());
    ASSERT_EQ(cmd.duration_arguments[1].count(),
              std::chrono::duration<long>((4 * 24 * 60 * 60) + 5 * 60 * 60 + 5 * 60 + 2).count());

    // Numeric arguments
    ASSERT_EQ(cmd.numeric_arguments[0], 3421);
}

TEST(Parse, endswith)
{
    ASSERT_TRUE(endswith("Hello!", "o!"));
    ASSERT_FALSE(endswith("Goodbye!", "o!"));
}

TEST(Parse, decompose_command_with_strings)
{
    auto cmd =
        decompose_command(std::string{"empty-thing "
                                      "\"hello! "
                                      "4d5lifetime-subscriptions-to-walmart! "
                                      "4d5h "
                                      "4d5h5m2s "
                                      "3421"});

    ASSERT_EQ(cmd.numeric_arguments.size(), 1);
    ASSERT_EQ(cmd.arguments.size(), 2);
    ASSERT_EQ(cmd.duration_arguments.size(), 2);

    // Arguments
    ASSERT_EQ(cmd.arguments[0], "\"hello!");
    ASSERT_EQ(cmd.arguments[1], "4d5lifetime-subscriptions-to-walmart!");

    // Duration arguments
    ASSERT_EQ(cmd.duration_arguments[0].count(),
              std::chrono::duration<long>((4 * 24 * 60 * 60) + 5 * 60 * 60).count());
    ASSERT_EQ(cmd.duration_arguments[1].count(),
              std::chrono::duration<long>((4 * 24 * 60 * 60) + 5 * 60 * 60 + 5 * 60 + 2).count());

    // Numeric arguments
    ASSERT_EQ(cmd.numeric_arguments[0], 3421);
}

TEST(Parse, decompose_command_with_good_strings)
{
    auto cmd =
        decompose_command(std::string{"empty-thing "
                                      "\"hello! "
                                      "4d5lifetime-subscriptions-to-walmart! "
                                      "4d5h\" "
                                      "4d5h5m2s "
                                      "3421"});

    EXPECT_EQ(cmd.numeric_arguments.size(), 1);
    EXPECT_EQ(cmd.arguments.size(), 1);
    EXPECT_EQ(cmd.duration_arguments.size(), 1);

    // Arguments
    EXPECT_EQ(cmd.arguments[0], "hello! 4d5lifetime-subscriptions-to-walmart! 4d5h");

    // Duration arguments
    EXPECT_EQ(cmd.duration_arguments[0].count(),
              std::chrono::duration<long>((4 * 24 * 60 * 60) + 5 * 60 * 60 + 5 * 60 + 2).count());

    // Numeric arguments
    EXPECT_EQ(cmd.numeric_arguments[0], 3421);
}
