#include <gtest/gtest.h>

#include <vector>

#include "bench/stats.h"

using shardcache::percentile_of_sorted;

TEST(PercentileTest, EmptyIsZero) {
    EXPECT_EQ(percentile_of_sorted({}, 50), 0.0);
}

TEST(PercentileTest, OneToHundred) {
    std::vector<double> v;
    for (int i = 1; i <= 100; ++i) {
        v.push_back(i);
    }
    EXPECT_EQ(percentile_of_sorted(v, 50), 50.0);
    EXPECT_EQ(percentile_of_sorted(v, 95), 95.0);
    EXPECT_EQ(percentile_of_sorted(v, 99), 99.0);
    EXPECT_EQ(percentile_of_sorted(v, 100), 100.0);
    EXPECT_EQ(percentile_of_sorted(v, 0), 1.0);
}

TEST(PercentileTest, TailIsDrivenByTheSlowestSamples) {
    //98 fast requests & 2 slow ones: p50 stays fast, p99 sees the slow tail
    std::vector<double> v(98, 100.0);
    v.push_back(5000.0);
    v.push_back(9000.0);
    EXPECT_EQ(percentile_of_sorted(v, 50), 100.0);
    EXPECT_EQ(percentile_of_sorted(v, 99), 5000.0);
}
