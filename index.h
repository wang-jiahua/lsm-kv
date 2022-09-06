#pragma once

#include <ctime>
#include <map>
#include <vector>
#include <string>
#include <memory>

#include "filter.h"

class IndexNode {
public:
    IndexNode(uint64_t offset,
              uint64_t length,
              std::time_t timestamp,
              bool deleted
    ) : offset_(offset),
        length_(length),
        timestamp_(timestamp),
        deleted_(deleted) {}

    [[nodiscard]] uint64_t get_offset() const { return offset_; }

    [[nodiscard]] uint64_t get_length() const { return length_; }

    [[nodiscard]] std::time_t get_timestamp() const { return timestamp_; }

    [[nodiscard]] bool is_deleted() const { return deleted_; }

private:
    uint64_t offset_;
    uint64_t length_;
    std::time_t timestamp_;
    bool deleted_;
};

typedef std::map <uint64_t, std::shared_ptr<IndexNode>> IndexTree;       // key -> info
typedef std::map <uint64_t, std::shared_ptr<IndexTree>, std::greater<>> IndexLevel; // filename -> tree

class Index {
public:
    Index();

    ~Index();

    void get(uint64_t key,
             int &level,
             uint64_t &filename,
             uint64_t &offset,
             uint64_t &length,
             bool &deleted) const;

    void put(uint64_t key,
             int level,
             const std::string &filename,
             uint64_t offset,
             uint64_t length,
             std::time_t timestamp,
             bool deleted);

    bool find(uint64_t key);

    void reset();

    void recover(Filter &filter);

    IndexLevel &get_level(int level) { return levels[level]; }

private:
    std::vector <IndexLevel> levels;
    const int maxLevel = 20;
};
