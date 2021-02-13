#include "utils/utils.hpp"
#include "gtest/gtest.h"

using namespace qb;

TEST(Utils, iequals)
{
    EXPECT_TRUE(iequals("thIsS", "thiss"));
    EXPECT_FALSE(iequals("thIsSs", "thiss"));
    EXPECT_TRUE(iequals("", ""));
    EXPECT_FALSE(iequals("string1", "string2"));
}
