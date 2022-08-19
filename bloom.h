#pragma once

#include <cstdint>
#include <cmath>
#include <bitset>

class BloomFilter {
public:
    BloomFilter();

    ~BloomFilter();

    void add(uint64_t key);

    [[nodiscard]] bool contains(uint64_t key) const;

    void reset();

private:
    // n == 1e6, p == 0.01%, m / n == -ln(p) / (ln(2) * ln(2)) == 20
    // k == m / n * ln(2) == 14
    static const uint64_t m = 2000000;
    static const uint64_t k = 14;
    std::bitset<m> bitset;
};
