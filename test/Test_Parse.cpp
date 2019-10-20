#include "parse.hpp"

#include "gtest/gtest.h"
#include <string>

using namespace qb::parse;

TEST(Parse, remove_non_cmd)
{
    std::string a{"!qb do something01"};
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
        for (auto && str : s)
        {
            ret +=str;
        }
        return ret;
    };
    
    EXPECT_EQ(vts(split(a)), vts(a_ans));
    EXPECT_EQ(vts(split(b)), vts(b_ans));
}
