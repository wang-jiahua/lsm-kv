#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>

#include "bloom.h"

class Filter {
public:
    Filter();

    ~Filter();

    void add(uint64_t key, int level, uint64_t filename);

    [[nodiscard]] bool contains(uint64_t key, int level, uint64_t filename) const;

    void reset();

    void recover();

private:
    const int maxLevel = 20;

    typedef std::map<uint64_t, BloomFilter> FilterLevel;

    std::vector<FilterLevel> filterLevels;
};
