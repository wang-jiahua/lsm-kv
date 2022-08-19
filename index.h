#pragma once

#include <ctime>
#include <map>
#include <vector>
#include <string>

#include "filter.h"

class IndexNode {

public:
    IndexNode(uint64_t _offset, uint64_t _length, std::time_t _timestamp, bool _deleted)
            : offset(_offset), length(_length), timestamp(_timestamp), deleted(_deleted) {
    }

public:
    uint64_t offset;
    uint64_t length;
    std::time_t timestamp;
    bool deleted;
};

typedef std::map<uint64_t, IndexNode *> IndexTree;                  // key -> info
typedef std::map<uint64_t, IndexTree *, std::greater<>> IndexLevel; // filename -> tree

class Index {
public:
    std::vector <IndexLevel> trees;
    const int maxLevel = 20;

public:
    Index();

    ~Index();

    void get(uint64_t key, int &level, uint64_t &filename, uint64_t &offset, uint64_t &length, bool &deleted) const;

    void put(uint64_t key, int level, const std::string &filename, uint64_t offset, uint64_t length,
             std::time_t timestamp, bool deleted);

    bool find(uint64_t key);

    void reset();

    void recover(Filter &filter);
};