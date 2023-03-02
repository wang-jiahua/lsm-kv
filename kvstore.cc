#include "kvstore.h"
#include "batch.h"
#include <future>
#include <functional>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

KVStore::KVStore(const std::string &dir) : KVStoreAPI(dir), dir_(dir), memtable(), index(dir), disk(dir), filter() {
    // maximum num of files are 2, 4, 8, 16, 32, ...
    for (int i = 0; i < maxLevel; ++i) {
        maxFileNums[i] = 1U << (i + 1);
    }
    recover_memtable();
    index.recover(filter);
}

KVStore::~KVStore() = default;

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s) {
    wal("put", key, s);
    memtable.put(key, s);
    // if memtable is full
    if (memtable.getSize() >= MAX_MEMTABLE_SIZE) {
        std::string walname = "wal";
        fs::path walpath = dir_;
        walpath /= walname;
        std::string immwalname = "immwal";
        fs::path immwalpath = dir_;
        immwalpath /= immwalname;
        // wait for the flush to finish
        flush.wait();
        // move memtable to immutable memtable
        imm_memtable = std::move(memtable);
        if (fs::exists(dir_)) {
            if (fs::exists(immwalpath)) {
                // remove immutable wal
                fs::remove(immwalname);
            }
            if (fs::exists(walpath)) {
                // rename wal to immwal
                fs::rename(walpath, immwalpath);
            }
        }
        flush = std::async(std::launch::async, std::bind(&KVStore::write_to_disk, this, 0, imm_memtable.traverse()));
        memtable.reset();
    }
}

/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key) const {
    bool mem_deleted = false;
    bool mem_found = true;
    const std::string &value = memtable.get(key, mem_deleted, mem_found);
    if (mem_found) {
        return value;
    }
    // if not found in memtable, find in immutable memtable
    if (imm_memtable.getSize() > 0) {
        bool imm_deleted = false;
        bool imm_found = true;
        const std::string &value = imm_memtable.get(key, imm_deleted, imm_found);
        if (imm_found) {
            return value;
        }
    }
    // if not found in immutable memtable, find in index
    int level = -1;
    uint64_t filename;
    uint64_t offset = UINT64_MAX;
    uint64_t length = 0U;
    bool index_deleted = false;
    index.get(key, level, filename, offset, length, index_deleted);
    if (offset == UINT64_MAX || index_deleted) {
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
void KVStore::scan(uint64_t lower, uint64_t upper, std::vector<std::pair<uint64_t, std::string>> &result) const {
    result.clear();
    std::map<uint64_t, const std::string> kv;
    std::map<std::pair<int, uint64_t>, Batch> batches;
    // a batch is a collection of information (key, offset, length) in a file
    // we combine multiple readings of a file into one
    // we must wait for the flush to finish, otherwise the immutable and file may be both empty
    flush.wait();
    for (uint64_t key = lower; key <= upper; key++) {
        bool mem_deleted = false;
        bool mem_found = true;
        const std::string &value = memtable.get(key, mem_deleted, mem_found);
        if (mem_found) {
            if (!value.empty()) {
                (void) kv.insert({key, value});
            }
            continue;
        }
        // if not found in memtable, find in immutable memtable
        if (imm_memtable.getSize() > 0) {
            bool imm_deleted = false;
            bool imm_found = true;
            const std::string &value = memtable.get(key, imm_deleted, imm_found);
            if (imm_found) {
                if (!value.empty()) {
                    (void) kv.insert({key, value});
                }
                continue;
            }
        }
        // if not found in immutable memtable, find in index
        int level = -1;
        uint64_t filename;
        uint64_t offset = UINT64_MAX;
        uint64_t length = 0U;
        bool index_deleted = false;
        index.get(key, level, filename, offset, length, index_deleted);
        if (offset == UINT64_MAX || index_deleted) {
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
    std::sort(result.begin(), result.end(), [](const std::pair<uint64_t, std::string> &a,
                                               const std::pair<uint64_t, std::string> &b) {
        return a.first < b.first;
    });
}

/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key) {
    wal("del", key, "");
    bool in_index = index.find(key);

    bool imm_deleted = false;
    bool imm_found = true;
    const std::string &s = imm_memtable.get(key, imm_deleted, imm_found);
    bool in_immutable = imm_found && !imm_deleted;
    bool success = memtable.del(key, in_index, in_immutable, !imm_found);
    // if memtable is full
    if (memtable.getSize() >= MAX_MEMTABLE_SIZE) {
        std::string walname = "wal";
        fs::path walpath = dir_;
        walpath /= walname;
        std::string immwalname = "immwal";
        fs::path immwalpath = dir_;
        immwalpath /= immwalname;
        // wait for the flush to finish
        flush.wait();
        // move memtable to immutable memtable
        imm_memtable = std::move(memtable);
        if (fs::exists(dir_)) {
            if (fs::exists(immwalpath)) {
                // remove immutable wal
                fs::remove(immwalname);
            }
            if (fs::exists(walpath)) {
                // rename wal to immwal
                fs::rename(walpath, immwalpath);
            }
        }
        flush = std::async(std::launch::async, std::bind(&KVStore::write_to_disk, this, 0, imm_memtable.traverse()));
        memtable.reset();
    }
    return success;
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset() {
    memtable.reset();
    index.reset();
    disk.reset();
    filter.reset();
}

void KVStore::print() const { memtable.print(); }

void KVStore::write_to_disk(int level, const Data &data) {
    std::time_t fileTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
    ).count();
    std::string filename = std::to_string(fileTimestamp);
    std::ofstream file;

    fs::path path = dir_;
    path /= std::to_string(level);
    (void) fs::create_directories(path);
    path /= filename;
    file.open(path, std::ios::out | std::ios::binary);

    std::vector<uint64_t> offsets;
    std::vector<uint64_t> lengths;

    for (auto &kv: data) {
        // record offset and length
        (void) offsets.emplace_back(file.tellp());
        (void) lengths.emplace_back(kv.value_.size());

        // write key and value
        (void) file.write((char *) (&(kv.key_)), sizeof(uint64_t));
        (void) file.write(kv.value_.c_str(), kv.value_.size());
        (void) file.write("\0", sizeof(char));

        (void) file.flush();
    }

    (void) file.flush();

    uint64_t n = data.size();

    for (uint64_t i = 0U; i < n; i++) {
        uint64_t key = data[i].key_;
        uint64_t offset = offsets[i];
        uint64_t length = lengths[i];

        // write key and offset
        (void) file.write(reinterpret_cast<char *>(&key), sizeof(uint64_t));
        (void) file.write(reinterpret_cast<char *>(&offset), sizeof(uint64_t));

        (void) file.flush();

        // sync with index
        index.put(key, level, filename, offset, length, std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
        ).count(), data[i].deleted_);

        // sync with filter
        filter.add(key, level, std::stoull(filename));
    }

    // write number of key-value pair for index recovery
    (void) file.write(reinterpret_cast<char *>(&n), sizeof(uint64_t));

    (void) file.flush();

    file.close();

    if (index.get_level(level).size() > maxFileNums[level]) {
        compact(level);
    }
}

void KVStore::compact(int level) {
    // record information to merge
    std::vector<MergeNode> toMerge;

    Range range;

    // number of files to merge in this level
    size_t num;
    if (level == 0) {
        num = index.get_level(level).size();
    } else {
        num = index.get_level(level).size() - maxFileNums[level];
    }

    auto treeKV1 = index.get_level(level).begin();

    for (size_t i = 0; i < num; i++) {
        uint64_t filename = treeKV1->first;
        auto tree = treeKV1->second;
        uint64_t lower = tree->begin()->first;
        uint64_t upper = tree->rbegin()->first;
        // ranges in this level
        range.push_back({lower, upper});
        (void) toMerge.emplace_back(MergeNode(level, filename, tree->begin()));
        treeKV1++;
    }

    // search files to merge in the next level
    for (auto &treeKV: index.get_level(level + 1)) {
        uint64_t filename = treeKV.first;
        auto tree = treeKV.second;
        uint64_t lower = tree->begin()->first;
        uint64_t upper = tree->rbegin()->first;
        if (inRange(lower, upper, range)) {
            (void) toMerge.emplace_back(MergeNode(level + 1, filename, tree->begin()));
        }
    }

    struct cmp {
        bool operator()(const MergeNode &a, const MergeNode &b) {
            return a.iter_->first > b.iter_->first;
        }
    };

    std::priority_queue<MergeNode, std::vector<MergeNode>, cmp> queue;

    for (auto &node: toMerge) {
        queue.push(node);
    }

    Data data;
    uint64_t size = 0U;

    while (!queue.empty()) {
        MergeNode latest = queue.top();
        auto tmp = latest.iter_;
        queue.pop();
        auto tree = index.get_level(latest.level_)[latest.filename_];
        if (++tmp != tree->end()) {
            queue.push(MergeNode(latest.level_, latest.filename_, tmp));
        }

        uint64_t key = latest.iter_->first;
        std::time_t timestamp = latest.iter_->second->get_timestamp();

        if (!queue.empty()) {
            MergeNode top = queue.top();
            // find the latest one with the same key
            while (!queue.empty() && queue.top().iter_ != latest.iter_ &&
                   queue.top().iter_->first == key) {
                if (queue.top().iter_->second->get_timestamp() >= timestamp) {
                    latest = queue.top();
                    timestamp = queue.top().iter_->second->get_timestamp();
                }

                auto tmp1 = queue.top();
                queue.pop();

                auto tree1 = index.get_level(tmp1.level_)[tmp1.filename_];
                auto tmp2 = tmp1.iter_;
                if (++tmp2 != tree1->end()) {
                    queue.push(MergeNode(tmp1.level_, tmp1.filename_, tmp2));
                }
                top = queue.top();
            }
        }

        // have found the latest one, record it
        key = latest.iter_->first;
        std::string value =
                disk.get(latest.level_, latest.filename_, latest.iter_->second->get_offset(),
                         latest.iter_->second->get_length());
        bool deleted = latest.iter_->second->is_deleted();

        (void) data.emplace_back(DataNode(key, value, deleted));
        size += sizeof(uint64_t) + value.size() + sizeof(uint64_t) + sizeof(uint64_t);

        if (size >= MAX_FILE_SIZE) {
            write_to_disk(level + 1, data);
            data.clear();
            size = 0U;
        }
    }
    if (!data.empty()) {
        write_to_disk(level + 1, data);
    }

    // delete merged files
    for (auto &node: toMerge) {
        (void) index.get_level(node.level_).erase(node.filename_);
        fs::path path = dir_;
        path /= std::to_string(node.level_);
        path /= std::to_string(node.filename_);
        (void) fs::remove(path);
    }
}

bool KVStore::inRange(uint64_t lower, uint64_t upper, const Range &range) {
    return std::any_of(range.begin(), range.end(), [&](const std::pair<uint64_t, uint64_t> &p) {
        return lower <= p.second && upper >= p.first;
    });
}

void KVStore::wal(const std::string &method, uint64_t key, const std::string &value) {
    std::string walname = "wal";
    std::ofstream file;
    fs::path path = dir_;
    if (!fs::exists(path)) {
        (void) fs::create_directories(path);
    }
    path /= walname;
    file.open(path, std::ios::out | std::ios::binary | std::ios::app); // append to the end of log

    (void) file.write(method.c_str(), method.size()); // method
    (void) file.write("\0", sizeof(char));

    (void) file.write((char *) (&key), sizeof(uint64_t)); // key

    uint64_t length = value.size();
    (void) file.write((char *) (&length), sizeof(uint64_t)); // length of value

    (void) file.write(value.c_str(), value.size()); // value
    (void) file.write("\0", sizeof(char));

    (void) file.flush();
    file.close();
}

void KVStore::recover_memtable() {
    if (!fs::exists(dir_)) {
        return;
    }
    std::vector<std::tuple<std::string, uint64_t, std::string>> ops; // method + key + value
    std::string immwalname = "immwal";
    fs::path immwalpath = dir_;
    immwalpath /= immwalname;
    if (fs::exists(immwalpath)) {
        std::ifstream immwal(immwalpath, std::ios::in | std::ios::binary);
        while (immwal) {
            std::string method;
            uint64_t key;
            uint64_t length;
            std::string value;
            // recover method
            std::getline(immwal, method, '\0');
            if (method.empty()) {
                break;
            }
            // recover key
            (void) immwal.read(reinterpret_cast<char *> (&key), sizeof(uint64_t));
            // recover length of value
            (void) immwal.read(reinterpret_cast<char *> (&length), sizeof(uint64_t));
            // recover value
            (void) std::getline(immwal, value, '\0');
            ops.emplace_back(method, key, value);
        }
    }
    std::string walname = "wal";
    fs::path walpath = dir_;
    walpath /= walname;
    if (fs::exists(walpath)) {
        std::ifstream wal(walpath, std::ios::in | std::ios::binary);
        while (wal) {
            std::string method;
            uint64_t key;
            uint64_t length;
            std::string value;
            // recover method
            std::getline(wal, method, '\0');
            if (method.empty()) {
                break;
            }
            // recover key
            (void) wal.read(reinterpret_cast<char *> (&key), sizeof(uint64_t));
            // recover length of value
            (void) wal.read(reinterpret_cast<char *> (&length), sizeof(uint64_t));
            // recover value
            (void) std::getline(wal, value, '\0');
            ops.emplace_back(method, key, value);
        }
    }
    for (auto &op: ops) {
        auto [method, key, value] = op;
        if (method == "put") {
            put(key, value);
        } else if (method == "del") {
            del(key);
        }
    }
}
