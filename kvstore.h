#pragma once

#include "disk.h"
#include "index.h"
#include "kvstore_api.h"
#include "skiplist.h"
#include "filter.h"

class KVStore : public KVStoreAPI {
private:
    SkipList MemTable;
    Index index;
    Disk disk;
    Filter filter;

    const uint64_t MAX_MEMTABLE_SIZE = 2 * 1024 * 1024; // 2MB

public:
    KVStore(const std::string &dir);

    ~KVStore();

    void put(uint64_t key, const std::string &s) override;

    std::string get(uint64_t key) override;

    bool del(uint64_t key) override;

    void reset() override;

    void print() const;
};
