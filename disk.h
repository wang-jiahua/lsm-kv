#pragma once

#include "index.h"
#include "data.h"
#include "skiplist.h"
#include "filter.h"

#include <queue>
#include <string>
#include <utility>
#include <vector>

using Range = std::vector<std::pair<uint64_t, uint64_t>>;

/**
 * information for compaction
 */
struct MergeNode {
    int level;
    uint64_t filename;
    IndexTree::iterator iter;

    MergeNode(int _level, uint64_t _filename, IndexTree::iterator _iter)
            : level(_level), filename(_filename), iter(_iter) {
    }
};

class Disk {
private:
    static const int maxLevel = 20;

    uint64_t maxFileNums[maxLevel]{};

    const uint64_t MAX_FILE_SIZE = 2 * 1024 * 1024; // 2MB

public:

    Disk();

    [[nodiscard]] std::string get(int level, uint64_t filename, uint64_t offset, uint64_t length) const;

    void put(int level, const Data &data, Index &index, Filter &filter);

    void reset();

private:
    void compact(Index &index, int level, Filter &filter);

    static bool inRange(uint64_t lower, uint64_t upper, const Range &range);
};