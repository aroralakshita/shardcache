#include <gtest/gtest.h>

#include "cache/store.h"

using shardcache::Store;


TEST(StoreTest, EmptyStoreHasSizeZero) {
    Store s;
    EXPECT_EQ(s.size(), 0u);
}

TEST(StoreTest, SetThenGetReturnsValue) {
    Store s;
    s.set("a", "1");
    EXPECT_EQ(s.get("a"), "1");
}

TEST(StoreTest, GetMissingKeyReturnsNullopt) {
    Store s;
    EXPECT_FALSE(s.get("nope").has_value());
}

TEST(StoreTest, OverwriteReplacesValueAndKeepsSize) {
    Store s;
    s.set("a", "old");
    s.set("a", "new");
    EXPECT_EQ(s.get("a"), "new");
    EXPECT_EQ(s.size(), 1u);
}

TEST(StoreTest, EmptyStringValueIsStoredNotMissing) {
    Store s;
    s.set("a", "");
    ASSERT_TRUE(s.get("a").has_value());
    EXPECT_EQ(s.get("a"), "");
}

TEST(StoreTest, RemoveExistingKeyReturnsTrueAndKeyIsGone) {
    Store s;
    s.set("a", "1");
    EXPECT_TRUE(s.remove("a"));
    EXPECT_FALSE(s.get("a").has_value());
    EXPECT_EQ(s.size(), 0u);
}

TEST(StoreTest, RemoveMissingKeyReturnsFalse) {
    Store s;
    EXPECT_FALSE(s.remove("nope"));
}

TEST(StoreTest, RemoveTwiceSecondReturnsFalse) {
    Store s;
    s.set("a", "1");
    EXPECT_TRUE(s.remove("a"));
    EXPECT_FALSE(s.remove("a"));
}

TEST(StoreTest, KeysAreIndependent) {
    Store s;
    s.set("a", "1");
    s.set("b", "2");
    s.remove("a");
    EXPECT_EQ(s.get("b"), "2");
    EXPECT_EQ(s.size(), 1u);
}
