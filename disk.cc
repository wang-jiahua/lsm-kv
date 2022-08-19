#include "disk.h"
#include "index.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <iostream>
#include <chrono>

namespace fs = std::filesystem;

std::string Disk::get(int level, uint64_t filename, uint64_t offset, uint64_t length) const {
    std::ifstream file("data/" + std::to_string(level) + "/" + std::to_string(filename),
                       std::ios::in | std::ios::binary);
    std::string value;
    file.seekg(offset + sizeof(uint64_t));
    std::getline(file, value, '\0');
    file.close();
    return value;
}

void Disk::put(int level, const Data &data, Index &index, Filter &filter) {
    std::time_t fileTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
    ).count();
    std::string filename = std::to_string(fileTimestamp);
    std::ofstream file;

    fs::create_directories("data/" + std::to_string(level));
    file.open("data/" + std::to_string(level) + "/" + filename, std::ios::out | std::ios::binary);

    std::vector<uint64_t> offsets;
    std::vector<uint64_t> lengths;

    for (auto &kv: data) {
        // record offset and length
        offsets.emplace_back(file.tellp());
        lengths.emplace_back(kv.value.size());

        // write key and value
        file.write((char *) (&(kv.key)), sizeof(uint64_t));
        file.write(kv.value.c_str(), kv.value.size());
        file.write("\0", sizeof(char));

        file.flush();
    }

    file.flush();

    uint64_t n = data.size();

    for (uint64_t i = 0; i < n; i++) {
        uint64_t key = data[i].key;
        uint64_t offset = offsets[i];
        uint64_t length = lengths[i];

        // write key and offset
        file.write(reinterpret_cast<char *>(&key), sizeof(uint64_t));
        file.write(reinterpret_cast<char *>(&offset), sizeof(uint64_t));

        file.flush();

        // sync with index
        index.put(key, level, filename, offset, length, std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
        ).count(), data[i].deleted);

        // sync with filter
        filter.add(key, level, std::stoull(filename));
    }

    // write number of key-value pair for index recovery
    file.write(reinterpret_cast<char *>(&n), sizeof(uint64_t));

    file.flush();

    file.close();

    if (index.trees[level].size() > maxFileNums[level]) {
        compact(index, level, filter);
    }
}

void Disk::compact(Index &index, int level, Filter &filter) {
    // record information to merge
    std::vector<MergeNode> toMerge;

    Range range;

    // number of files to merge in this level
    size_t num = 0;
    if (level == 0) {
        num = index.trees[level].size();
    } else {
        num = index.trees[level].size() - maxFileNums[level];
    }

    auto treeKV1 = index.trees[level].begin();

    for (size_t i = 0; i < num; i++) {
        uint64_t filename = treeKV1->first;
        IndexTree *tree = treeKV1->second;
        uint64_t lower = tree->begin()->first;
        uint64_t upper = tree->rbegin()->first;
        // ranges in this level
        range.push_back({lower, upper});
        toMerge.emplace_back(MergeNode(level, filename, tree->begin()));
        treeKV1++;
    }

    // search files to merge in the next level
    for (auto &treeKV: index.trees[level + 1]) {
        uint64_t filename = treeKV.first;
        IndexTree *tree = treeKV.second;
        uint64_t lower = tree->begin()->first;
        uint64_t upper = tree->rbegin()->first;
        if (inRange(lower, upper, range)) {
            toMerge.emplace_back(MergeNode(level + 1, filename, tree->begin()));
        }
    }

    struct cmp {
        bool operator()(const MergeNode &a, const MergeNode &b) {
            return a.iter->first > b.iter->first;
        }
    };

    std::priority_queue<MergeNode, std::vector<MergeNode>, cmp> queue;

    for (auto &node: toMerge) {
        queue.push(node);
    }

    Data data;
    uint64_t size = 0;

    while (!queue.empty()) {
        MergeNode latest = queue.top();
        auto tmp = latest.iter;
        queue.pop();
        IndexTree *tree = index.trees[latest.level][latest.filename];
        if (++tmp != tree->end()) {
            queue.push(MergeNode(latest.level, latest.filename, tmp));
        }

        uint64_t key = latest.iter->first;
        std::time_t timestamp = latest.iter->second->timestamp;

        if (!queue.empty()) {
            MergeNode top = queue.top();
            // find the latest one with the same key
            while (!queue.empty() && queue.top().iter != latest.iter && queue.top().iter->first == key) {
                if (queue.top().iter->second->timestamp >= timestamp) {
                    latest = queue.top();
                    timestamp = queue.top().iter->second->timestamp;
                }

                auto tmp1 = queue.top();
                queue.pop();

                IndexTree *tree1 = index.trees[tmp1.level][tmp1.filename];
                auto tmp2 = tmp1.iter;
                if (++tmp2 != tree1->end()) {
                    queue.push(MergeNode(tmp1.level, tmp1.filename, tmp2));
                }
                top = queue.top();
            }
        }

        // have found the latest one, record it
        key = latest.iter->first;
        std::string value =
                get(latest.level, latest.filename, latest.iter->second->offset, latest.iter->second->length);
        bool deleted = latest.iter->second->deleted;

        data.emplace_back(DataNode(key, value, deleted));
        size += sizeof(uint64_t) + value.size() + sizeof(uint64_t) + sizeof(uint64_t);

        if (size >= MAX_FILE_SIZE) {
            put(level + 1, data, index, filter);
            data.clear();
            size = 0;
        }
    }
    if (!data.empty())
        put(level + 1, data, index, filter);

    // delete merged files
    for (auto &node: toMerge) {
        index.trees[node.level].erase(node.filename);
        fs::remove("data/" + std::to_string(node.level) + "/" + std::to_string(node.filename));
    }
}

bool Disk::inRange(uint64_t lower, uint64_t upper, const Range &range) {
    for (auto &r: range) {
        uint64_t range_lower = r.first;
        uint64_t range_upper = r.second;
        if (lower <= range_upper && upper >= range_lower) {
            return true;
        }
    }
    return false;
}

Disk::Disk() {
    // maximum num of files are 2, 4, 8, 16, 32, ...
    for (int i = 0; i < maxLevel; ++i) {
        maxFileNums[i] = 1 << (i + 1);
    }
}

void Disk::reset() {

}
