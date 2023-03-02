#include "disk.h"
#include "index.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <iostream>
#include <chrono>

namespace fs = std::filesystem;

std::string Disk::get(int level, uint64_t filename, uint64_t offset, uint64_t length) const {
    fs::path path = dir_;
    path /= std::to_string(level);
    path /= std::to_string(filename);
    std::ifstream file(path, std::ios::in | std::ios::binary);
    std::string value;
    (void) file.seekg(offset + sizeof(uint64_t));
    (void) std::getline(file, value, '\0');
    file.close();
    return value;
}

void Disk::get(const Batch &batch, std::map<uint64_t, const std::string> &kv) const {
    fs::path path = dir_;
    path /= std::to_string(batch.level_);
    path /= std::to_string(batch.filename_);
    std::ifstream file(path, std::ios::in | std::ios::binary);
    for (auto &it: batch.infos_) {
        auto [key, offset, length] = it;
        std::string value;
        (void) file.seekg(offset + sizeof(uint64_t));
        (void) std::getline(file, value, '\0');
        (void) kv.insert({key, value});
    }
    file.close();
}

Disk::Disk(const std::string &dir) : dir_(dir) {}

void Disk::reset() {

}
