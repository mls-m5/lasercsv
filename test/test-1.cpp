#include "lasercsv/csv.hpp"
#include "mls-unit-test/unittest.h"

TEST_SUIT_BEGIN

TEST_CASE("basic") {
    auto table = lasercsv::Table::fromString(R"_(
1,2
3,4
8,2
)_",
                                             "test.csv");

    EXPECT_EQ(table.width(), 2);
    EXPECT_EQ(table.height(), 3);
}

TEST_CASE("trailing comas") {
    auto table = lasercsv::Table::fromString(R"_(
1,2,
3,4,
8,2;
)_",
                                             "test.csv");

    EXPECT_EQ(table.width(), 2);
    EXPECT_EQ(table.height(), 3);
}

TEST_CASE("quotation") {
    auto table = lasercsv::Table::fromString(R"_(
1,2,
3,"hello",
"there",2;
)_",
                                             "test.csv");

    EXPECT_EQ(table.width(), 2);
    EXPECT_EQ(table.height(), 3);

    EXPECT_EQ(table.rows().at(1).at(1).str(), "hello");
    EXPECT_EQ(table.rows().at(2).at(0).str(), "there");
}

TEST_SUIT_END
