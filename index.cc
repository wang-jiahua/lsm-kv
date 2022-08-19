#include "index.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <fstream>

namespace fs = std::filesystem;

void Index::get(uint64_t key, int &level, uint64_t &filename, uint64_t &offset, uint64_t &length, bool &deleted) const {
    if (trees.empty()) {
        return;
    }
    // search top-down
    for (level = 0; level < maxLevel; ++level) {
        // search from the latest one
        for (auto &indexKV: trees[level]) {
            uint64_t timestamp = indexKV.first;
            IndexTree *indexTree = indexKV.second;
            auto iter = indexTree->find(key);
            // if find key in this file
            if (iter != indexTree->end()) {
                filename = timestamp;
                offset = iter->second->offset;
                length = iter->second->length;
                deleted = iter->second->deleted;
                return;
            }
        }
    }
}

bool Index::find(uint64_t key) {
    for (auto &level: trees) {
        for (auto &pair: level) {
            IndexTree *tree = pair.second;
            auto iter = tree->find(key);
            // if key in disk and not marked as deleted
            if (iter != tree->end() && !(iter->second->deleted)) {
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
    if (trees[level].count(std::stoull(filename)) == 0) {
        trees[level].insert({std::stoull(filename), new IndexTree()});
    }
    IndexTree *tree = trees[level][std::stoull(filename)];
    tree->insert({key, new IndexNode(offset, length, timestamp, deleted)});
}

Index::Index() {
    trees = std::vector<IndexLevel>(maxLevel);
}

Index::~Index() {

}

void Index::reset() {
    for (auto &level: trees) {
        for (auto &pair: level) {
            IndexTree *tree = pair.second;
            for (auto &kv: *tree) {
                IndexNode *node = kv.second;
                delete node;
            }
            delete tree;
        }
    }
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
        file.seekg(-sizeof(uint64_t), file.end);
        file.read(reinterpret_cast<char *>(&n), sizeof(uint64_t));

        std::size_t pos = path.find('/');
        path = path.substr(pos + 1, path.size());

        for (uint64_t i = 1; i <= n; i++) {
            // recover key
            uint64_t key;
            file.seekg((int) (-sizeof(uint64_t) * (1 + 2 * i)), file.end);
            file.read(reinterpret_cast<char *>(&key), sizeof(uint64_t));
            // recover offset
            uint64_t offset = 0;
            file.seekg((int) (-sizeof(uint64_t) * (2 * i)), std::ios_base::end);
            file.read(reinterpret_cast<char *>(&offset), sizeof(uint64_t));
            // recover value
            std::string s;
            file.seekg(offset + sizeof(uint64_t));
            std::getline(file, s, '\0');

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
