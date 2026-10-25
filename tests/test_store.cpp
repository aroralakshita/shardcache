#include <gtest/gtest.h>

#include "cache/store.h"

using shardcache::Store;


TEST(StoreTest, EmptyStoreHasSizeZero) {
    Store s(10);
    EXPECT_EQ(s.size(), 0u);
}

TEST(StoreTest, SetThenGetReturnsValue) {
    Store s(10);
    s.set("a", "1");
    EXPECT_EQ(s.get("a"), "1");
}

TEST(StoreTest, GetMissingKeyReturnsNullopt) {
    Store s(10);
    EXPECT_FALSE(s.get("nope").has_value());
}

TEST(StoreTest, OverwriteReplacesValueAndKeepsSize) {
    Store s(10);
    s.set("a", "old");
    s.set("a", "new");
    EXPECT_EQ(s.get("a"), "new");
    EXPECT_EQ(s.size(), 1u);
}

TEST(StoreTest, EmptyStringValueIsStoredNotMissing) {
    Store s(10);
    s.set("a", "");
    ASSERT_TRUE(s.get("a").has_value());
    EXPECT_EQ(s.get("a"), "");
}

TEST(StoreTest, RemoveExistingKeyReturnsTrueAndKeyIsGone) {
    Store s(10);
    s.set("a", "1");
    EXPECT_TRUE(s.remove("a"));
    EXPECT_FALSE(s.get("a").has_value());
    EXPECT_EQ(s.size(), 0u);
}

TEST(StoreTest, RemoveMissingKeyReturnsFalse) {
    Store s(10);
    EXPECT_FALSE(s.remove("nope"));
}

TEST(StoreTest, RemoveTwiceSecondReturnsFalse) {
    Store s(10);
    s.set("a", "1");
    EXPECT_TRUE(s.remove("a"));
    EXPECT_FALSE(s.remove("a"));
}

TEST(StoreTest, KeysAreIndependent) {
    Store s(10);
    s.set("a", "1");
    s.set("b", "2");
    s.remove("a");
    EXPECT_EQ(s.get("b"), "2");
    EXPECT_EQ(s.size(), 1u);
}

//LRU eviction

TEST(LruTest, EvictsLeastRecentlyUsedWhenFull) {
    Store s(2);
    s.set("a", "1");
    s.set("b", "2");
    s.set("c", "3");
    EXPECT_FALSE(s.get("a").has_value());
    EXPECT_EQ(s.get("b"), "2");
    EXPECT_EQ(s.get("c"), "3");
    EXPECT_EQ(s.size(), 2u);
}

TEST(LruTest, GetMakesKeyMostRecentlyUsed) {
    Store s(2);
    s.set("a", "1");
    s.set("b", "2");
    s.get("a");
    s.set("c", "3");
    EXPECT_FALSE(s.get("b").has_value());
    EXPECT_EQ(s.get("a"), "1");
    EXPECT_EQ(s.get("c"), "3");
}

TEST(LruTest, OverwriteDoesNotEvictAndCountsAsUse) {
    Store s(2);
    s.set("a", "old");
    s.set("b", "2");
    s.set("a", "new");
    EXPECT_EQ(s.size(), 2u);
    s.set("c", "3");
    EXPECT_FALSE(s.get("b").has_value());
    EXPECT_EQ(s.get("a"), "new");
}

TEST(LruTest, GetMissingKeyDoesNotChangeOrder) {
    Store s(2);
    s.set("a", "1");
    s.set("b", "2");
    s.get("nope"); 
    s.set("c", "3");
    EXPECT_FALSE(s.get("a").has_value());
    EXPECT_EQ(s.get("b"), "2");
}

TEST(LruTest, RemoveFreesASlot) {
    Store s(2);
    s.set("a", "1");
    s.set("b", "2");
    s.remove("a");
    s.set("c", "3");
    EXPECT_EQ(s.get("b"), "2");
    EXPECT_EQ(s.get("c"), "3");
    EXPECT_EQ(s.size(), 2u);
}

TEST(LruTest, CapacityOneKeepsOnlyLatestKey) {
    Store s(1);
    s.set("a", "1");
    s.set("b", "2");
    EXPECT_FALSE(s.get("a").has_value());
    EXPECT_EQ(s.get("b"), "2");
    EXPECT_EQ(s.size(), 1u);
}

TEST(LruTest, CapacityZeroStoresNothing) {
    Store s(0);
    s.set("a", "1");
    EXPECT_FALSE(s.get("a").has_value());
    EXPECT_EQ(s.size(), 0u);
}
