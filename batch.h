#pragma once

#include <set>

class Batch {
public:
    int level_;
    uint64_t filename_;

    using Info = std::tuple<uint64_t, uint64_t, uint64_t>; // key, offset, length

    std::set<Info> infos_;

    Batch(int level, uint64_t filename) : level_(level), filename_(filename) {}
};