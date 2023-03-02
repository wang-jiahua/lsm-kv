#pragma once

#include "index.h"
#include "data.h"
#include "skiplist.h"
#include "filter.h"
#include "batch.h"

#include <queue>
#include <string>
#include <utility>
#include <vector>

using Range = std::vector<std::pair<uint64_t, uint64_t>>;

/**
 * information for compaction
 */
class MergeNode {
public:
    MergeNode(int level, uint64_t filename, IndexTree::iterator iter)
            : level_(level), filename_(filename), iter_(iter) {}

private:
    friend class Disk;

    friend class KVStore;

    int level_;
    uint64_t filename_;
    IndexTree::iterator iter_;
};

class Disk {
private:
    const std::string &dir_;

    static const int maxLevel = 20;

public:
    Disk(const std::string &dir);

    [[nodiscard]] std::string get(int level, uint64_t filename, uint64_t offset, uint64_t length) const;

    void get(const Batch &batch, std::map<uint64_t, const std::string> &kv) const;

    void reset();
};
