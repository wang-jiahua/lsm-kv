/**
 * For converting memtable to sstable. A DataNode represent a key-value pair in the memtable
 */

#pragma once

#include <string>
#include <utility>
#include <vector>

class DataNode {
public:
    DataNode(uint64_t key, std::string value, bool deleted) :
            key_(key),
            value_(std::move(value)),
            deleted_(deleted) {}

    uint64_t key_;
    std::string value_;
    bool deleted_;
};

using Data = std::vector<DataNode>;
