#include "kvstore.h"
#include "batch.h"

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
std::string KVStore::get(uint64_t key) const {
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
 * Gets the key-value pairs between [lower, upper].
 * @param lower the lower bound of the key range (inclusive)
 * @param upper the upper bound of the key range (inclusive)
 * @param result the result vector to be filled
 */
void KVStore::scan(uint64_t lower, uint64_t upper, std::vector<std::pair<uint64_t, const std::string>> &result) const {
    result.clear();
    std::map<uint64_t, const std::string> kv;
    std::map<std::pair<int, uint64_t>, Batch> batches;
    // a batch is a collection of information (key, offset, length) in a file
    // we combine multiple readings of a file into one
    for (uint64_t key = lower; key <= upper; key++) {
        bool deleted = false;
        bool found = true;
        const std::string &s = MemTable.get(key, deleted, found);
        if (found) {
            if (!s.empty()) {
                (void) kv.insert({key, s});
            }
            continue;
        }
        // if not found in memtable, find in index
        int level = -1;
        uint64_t filename;
        uint64_t offset = UINT64_MAX;
        uint64_t length = 0U;
        index.get(key, level, filename, offset, length, deleted);
        if (offset == UINT64_MAX || deleted) {
            continue;
        }
        if (!filter.contains(key, level, filename)) {
            continue;
        }
        if (batches.count({level, filename}) == 0U) {
            (void) batches.insert({{level, filename}, Batch(level, filename)});
        }
        (void) batches.at({level, filename}).infos_.insert({key, offset, length});
    }
    for (auto &it: batches) {
        disk.get(it.second, kv);
    }
    for (auto &it: kv) {
        (void) result.emplace_back(it.first, it.second);
    }
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
