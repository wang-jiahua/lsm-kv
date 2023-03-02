#pragma once

#include "disk.h"
#include "index.h"
#include "kvstore_api.h"
#include "skiplist.h"
#include "filter.h"
#include <future>

class KVStore : public KVStoreAPI {
private:
    const std::string dir_;
    SkipList memtable;
    SkipList imm_memtable;
    Index index;
    Disk disk;
    Filter filter;
    std::future<void> flush = std::async(std::launch::async, []() { return; });

    const uint64_t MAX_MEMTABLE_SIZE = 2U * 1024U * 1024U; // 2MB

    uint64_t maxFileNums[maxLevel]{};

    const uint64_t MAX_FILE_SIZE = 2U * 1024U * 1024U; // 2MB

public:
    explicit KVStore(const std::string &dir);

    ~KVStore();

    void put(uint64_t key, const std::string &s) override;

    [[nodiscard]] std::string get(uint64_t key) const override;

    void
    scan(uint64_t lower, uint64_t upper, std::vector<std::pair<uint64_t, std::string>> &result) const override;

    bool del(uint64_t key) override;

    void reset() override;

    void print() const;

    void write_to_disk(int level, const Data &data);

    void compact(int level);

    bool inRange(uint64_t lower, uint64_t upper, const Range &range);
};
