/**
 * For converting memtable to sstable. A DataNode represent a key-value pair in the memtable
 */

#pragma once

#include <string>
#include <utility>
#include <vector>

struct DataNode {
    uint64_t key;
    std::string value;
    bool deleted;

    DataNode(uint64_t _key, std::string _value, bool _deleted) :
            key(_key),
            value(std::move(_value)),
            deleted(_deleted) {}
};

using Data = std::vector<DataNode>;
