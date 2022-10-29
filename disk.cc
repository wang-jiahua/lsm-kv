#include "disk.h"
#include "index.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <utility>

namespace fs = std::filesystem;

std::string Disk::get(int level, uint64_t filename, uint64_t offset, uint64_t length) const {
    std::ifstream file(dir_ + std::to_string(level) + "/" + std::to_string(filename),
                       std::ios::in | std::ios::binary);
    std::string value;
    (void) file.seekg(offset + sizeof(uint64_t));
    (void) std::getline(file, value, '\0');
    file.close();
    return value;
}

void Disk::get(const Batch &batch, std::map<uint64_t, const std::string> &kv) const {
    std::ifstream file(dir_ + std::to_string(batch.level_) + "/" + std::to_string(batch.filename_),
                       std::ios::in | std::ios::binary);
    for (auto &it: batch.infos_) {
        auto [key, offset, length] = it;
        std::string value;
        (void) file.seekg(offset + sizeof(uint64_t));
        (void) std::getline(file, value, '\0');
        (void) kv.insert({key, value});
    }
    file.close();
}

void Disk::put(int level, const Data &data, Index &index, Filter &filter) {
    std::time_t fileTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
    ).count();
    std::string filename = std::to_string(fileTimestamp);
    std::ofstream file;

    (void) fs::create_directories(dir_ + std::to_string(level));
    file.open(dir_ + std::to_string(level) + "/" + filename, std::ios::out | std::ios::binary);

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
        compact(index, level, filter);
    }
}

void Disk::compact(Index &index, int level, Filter &filter) {
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
                get(latest.level_, latest.filename_, latest.iter_->second->get_offset(),
                    latest.iter_->second->get_length());
        bool deleted = latest.iter_->second->is_deleted();

        (void) data.emplace_back(DataNode(key, value, deleted));
        size += sizeof(uint64_t) + value.size() + sizeof(uint64_t) + sizeof(uint64_t);

        if (size >= MAX_FILE_SIZE) {
            put(level + 1, data, index, filter);
            data.clear();
            size = 0U;
        }
    }
    if (!data.empty()) {
        put(level + 1, data, index, filter);
    }

    // delete merged files
    for (auto &node: toMerge) {
        (void) index.get_level(node.level_).erase(node.filename_);
        (void) fs::remove(dir_ + std::to_string(node.level_) + "/" + std::to_string(node.filename_));
    }
}

bool Disk::inRange(uint64_t lower, uint64_t upper, const Range &range) {
    return std::any_of(range.begin(), range.end(), [&](const std::pair<uint64_t, uint64_t> &p) {
        return lower <= p.second && upper >= p.first;
    });
}

Disk::Disk(const std::string &dir) : dir_(dir) {
    // maximum num of files are 2, 4, 8, 16, 32, ...
    for (int i = 0; i < maxLevel; ++i) {
        maxFileNums[i] = 1U << (i + 1);
    }
}

void Disk::reset() {

}
