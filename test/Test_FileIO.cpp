#include "fileio.hpp"

#include "config.hpp"
#include "parse.hpp"
#include "utils.hpp"
#include "gtest/gtest.h"
#include <string>
#include <vector>

using namespace qb::fileio;

TEST(FileIO, GetSets)
{
    // This is shockingly useful for testing
    qb::config::get_config_()["skribbl_path"] = "../test/test_skribbl.noedit.json";

    // First check if the file exists
    EXPECT_FALSE(skribbl::storage_exists());

    // Now create the file
    std::vector<std::string> words_1 = {"this", "is a", "set of", "words"};
    add_default(words_1);

    std::vector<std::string> wordset_1 = {"set", "is a", "set of", "worders"};
    add_to_set("wordset1", wordset_1);

    auto wordset_1_back = get_set("wordset1");
    EXPECT_EQ(qb::parse::concatenate(wordset_1_back), qb::parse::concatenate(wordset_1));
    EXPECT_TRUE(qb::range_eq(wordset_1_back, wordset_1));

    // Test exactly what we have in src/bot.cpp for redundancy
    auto components = qb::parse::split(" recall:wordset1", ':');
    components.erase(components.begin());
    EXPECT_EQ(qb::parse::concatenate(qb::fileio::get_set(components.at(0)), ","), "set,is a,set of,worders");

    components = qb::parse::split(" recall:wordset1:default", ':');
    components.erase(components.begin());
    EXPECT_EQ(qb::parse::concatenate(qb::fileio::get_sets(components), ","), "set,is a,set of,worders,this,is a,set of,words");
}
