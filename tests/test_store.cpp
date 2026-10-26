#include <gtest/gtest.h>

#include "cache/store.h"

using shardcache::Store;
using namespace std::chrono_literals;  // lets tests write 30s, 500ms, ...

// A clock the test controls: time only moves when the test calls advance().
// Usage:
//   FakeClock clock;
//   Store s(10, [&clock] { return clock.now; });
//   s.set("a", "1", 30s);
//   clock.advance(31s);
struct FakeClock {
    Store::TimePoint now{};
    void advance(std::chrono::milliseconds d) { now += d; }
};


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

//TTL: lazy expiry, fake clock

TEST(TtlTest, KeyWithoutTtlNeverExpires) {
    FakeClock clock;
    Store s(10, [&clock] { return clock.now; });
    s.set("a", "1");
    clock.advance(1000h);
    EXPECT_EQ(s.get("a"), "1");
}

TEST(TtlTest, ReadBeforeDeadlineDoesNotExtendIt) {
    FakeClock clock;
    Store s(10, [&clock] { return clock.now; });
    s.set("a", "1", 30s);
    clock.advance(20s);
    EXPECT_EQ(s.get("a"), "1"); 
    clock.advance(11s);
    EXPECT_FALSE(s.get("a").has_value()); 
}

TEST(TtlTest, KeyIsAliveAtExactDeadlineAndGoneJustAfter) {
    FakeClock clock;
    Store s(10, [&clock] { return clock.now; });
    s.set("a", "1", 30s);
    clock.advance(30s);
    EXPECT_EQ(s.get("a"), "1"); 
    clock.advance(1ms);
    EXPECT_FALSE(s.get("a").has_value());
}

TEST(TtlTest, OverwriteWithoutTtlClearsDeadline) {
    FakeClock clock;
    Store s(10, [&clock] { return clock.now; });
    s.set("a", "old", 30s);
    s.set("a", "new");  
    clock.advance(1h);
    EXPECT_EQ(s.get("a"), "new");
}

TEST(TtlTest, OverwriteWithTtlUsesNewDeadline) {
    FakeClock clock;
    Store s(10, [&clock] { return clock.now; });
    s.set("a", "old", 30s);  
    clock.advance(20s);
    s.set("a", "new", 30s);  
    clock.advance(20s);      
    EXPECT_EQ(s.get("a"), "new");
}

TEST(TtlTest, OverwriteAddsTtlToKeyThatHadNone) {
    FakeClock clock;
    Store s(10, [&clock] { return clock.now; });
    s.set("a", "1");
    s.set("a", "2", 10s);
    clock.advance(11s);
    EXPECT_FALSE(s.get("a").has_value());
}

TEST(TtlTest, ExpiredKeyCountsInSizeUntilItIsRead) {
    FakeClock clock;
    Store s(10, [&clock] { return clock.now; });
    s.set("a", "1", 10s);
    s.set("b", "2");
    clock.advance(11s);
    EXPECT_EQ(s.size(), 2u);  
    EXPECT_FALSE(s.get("a").has_value());
    EXPECT_EQ(s.size(), 1u); 
}
