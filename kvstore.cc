#include "kvstore.h"
#include <string>

KVStore::KVStore(const std::string &dir) : KVStoreAPI(dir) {
    index.recover(filter);
}

KVStore::~KVStore() = default;

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s) {
    MemTable.put(key, s);
    // if memtable is full
    if (MemTable.getSize() >= MAX_MEMTABLE_SIZE) {
        disk.put(0, MemTable.traverse(), index, filter);
        MemTable.reset();
    }
}

/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key) {
    bool deleted = false;
    bool found = true;
    const std::string &s = MemTable.get(key, deleted, found);
    if (found) {
        return s;
    }
    // if not found in memtable, find in index
    int level = -1;
    uint64_t filename;
    uint64_t offset = UINT64_MAX;
    uint64_t length = 0U;
    index.get(key, level, filename, offset, length, deleted);
    if (offset == UINT64_MAX || deleted) {
        return {};
    }
    if (!filter.contains(key, level, filename)) {
        return {};
    }
    // get in disk
    return disk.get(level, filename, offset, length);
}

/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key) {
    bool inIndex = index.find(key);
    bool success = MemTable.del(key, inIndex);
    // if memtable is full
    if (MemTable.getSize() >= MAX_MEMTABLE_SIZE) {
        disk.put(0, MemTable.traverse(), index, filter);
        MemTable.reset();
    }
    return success;
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset() {
    MemTable.reset();
    index.reset();
    disk.reset();
    filter.reset();
}

void KVStore::print() const { MemTable.print(); }
