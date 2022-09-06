#include "index.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <fstream>

namespace fs = std::filesystem;

void Index::get(uint64_t key, int &level, uint64_t &filename, uint64_t &offset, uint64_t &length, bool &deleted) const {
    if (levels.empty()) {
        return;
    }
    // search top-down
    for (level = 0; level < maxLevel; ++level) {
        // search from the latest one
        for (auto &indexKV: levels[level]) {
            uint64_t timestamp = indexKV.first;
            std::shared_ptr<IndexTree> indexTree = indexKV.second;
            auto iter = indexTree->find(key);
            // if key found in this file
            if (iter != indexTree->end()) {
                filename = timestamp;
                offset = iter->second->get_offset();
                length = iter->second->get_length();
                deleted = iter->second->is_deleted();
                return;
            }
        }
    }
}

bool Index::find(uint64_t key) {
    for (auto &level: levels) {
        for (auto &pair: level) {
            std::shared_ptr<IndexTree> tree = pair.second;
            auto iter = tree->find(key);
            // if key in disk and not marked as deleted
            if (iter != tree->end() && !(iter->second->is_deleted())) {
                return true;
            }
        }
    }
    return false;
}

void
Index::put(uint64_t key, int level, const std::string &filename, uint64_t offset, uint64_t length,
           std::time_t timestamp, bool deleted) {
    if (level > maxLevel) {
        return;
    }
    // if file not existed, create it
    if (levels[level].count(std::stoull(filename)) == 0) {
        (void) levels[level].insert({std::stoull(filename), std::make_shared<IndexTree>()});
    }
    auto &tree = levels[level][std::stoull(filename)];
    (void) tree->insert({key, std::make_shared<IndexNode>(offset, length, timestamp, deleted)});
}

Index::Index() {
    levels = std::vector<IndexLevel>(maxLevel, IndexLevel());
}

Index::~Index() = default;

void Index::reset() {
    levels.clear();
    levels = std::vector<IndexLevel>(maxLevel, IndexLevel());
}

void Index::recover(Filter &filter) {
    if (!fs::exists("data")) {
        return;
    }
    for (auto &p: fs::recursive_directory_iterator("data")) {
        if (fs::is_directory(p)) {
            continue;
        }
        std::string path = p.path().string();
        std::ifstream file(path, std::ios::in | std::ios::binary);

        // get number of key-value pairs in the last 2 bytes
        uint64_t n = 0;
        (void) file.seekg(-sizeof(uint64_t), file.end);
        (void) file.read(reinterpret_cast<char *>(&n), sizeof(uint64_t));

        std::size_t pos = path.find('/');
        path = path.substr(pos + 1, path.size());

        for (uint64_t i = 1; i <= n; i++) {
            // recover key
            uint64_t key;
            (void) file.seekg((int) (-sizeof(uint64_t) * (1 + 2 * i)), std::ios_base::end);
            (void) file.read(reinterpret_cast<char *>(&key), sizeof(uint64_t));
            // recover offset
            uint64_t offset = 0;
            (void) file.seekg((int) (-sizeof(uint64_t) * (2 * i)), std::ios_base::end);
            (void) file.read(reinterpret_cast<char *>(&offset), sizeof(uint64_t));
            // recover value
            std::string s;
            (void) file.seekg(offset + sizeof(uint64_t));
            (void) std::getline(file, s, '\0');

            pos = path.find('/');
            int level = std::stoi(path.substr(0, pos));
            std::string filename = path.substr(pos + 1, path.size());

            put(key, level, filename, offset, s.size(), std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
            ).count(), false);

            // sync with filter
            filter.add(key, level, std::stoull(filename));
        }
        file.close();
    }
}
