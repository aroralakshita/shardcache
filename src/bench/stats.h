#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

namespace shardcache {

//nearest-rank percentile of an already sorted vector: the smallest sample such that at
//least p% of all samples are <= it. p is in [0, 100]. returns 0 for no samples
inline double percentile_of_sorted(const std::vector<double>& sorted, double p) {
    if (sorted.empty()) {
        return 0.0;
    }
    auto rank = static_cast<std::size_t>(std::ceil(p / 100.0 * static_cast<double>(sorted.size())));
    if (rank == 0) {
        rank = 1;
    }
    if (rank > sorted.size()) {
        rank = sorted.size();
    }
    return sorted[rank - 1];
}

}
