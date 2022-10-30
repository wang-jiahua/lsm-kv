
#include "filter.h"
#include <iostream>

Filter::Filter() {
    filterLevels = std::vector<FilterLevel>(maxLevel);
}

Filter::~Filter() = default;

void Filter::add(uint64_t key, int level, uint64_t filename) {
    if (filterLevels[level].count(filename) == 0U) {
        (void) filterLevels[level].insert({filename, BloomFilter()});
    }
    BloomFilter &bloomFilter = filterLevels[level][filename];
    bloomFilter.add(key);
}

bool Filter::contains(uint64_t key, int level, uint64_t filename) const {
    if (filterLevels[level].count(filename) == 0U) {
        return false;
    }
    const BloomFilter &bloomFilter = filterLevels[level].at(filename);
    return bloomFilter.contains(key);
}

void Filter::reset() {
    filterLevels = std::vector<FilterLevel>(maxLevel);
}

void Filter::recover() {

}
