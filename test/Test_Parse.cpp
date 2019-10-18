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
