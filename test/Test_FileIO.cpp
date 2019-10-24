#include "fileio.hpp"

#include "gtest/gtest.h"
#include <string>
#include <vector>
#include "config.hpp"

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
}
