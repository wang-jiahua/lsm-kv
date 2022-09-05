#include "bloom.h"
#include "util/MurmurHash3.h"

BloomFilter::BloomFilter() {
    reset();
}

BloomFilter::~BloomFilter() = default;

void BloomFilter::add(uint64_t key) {
    uint32_t h;
    MurmurHash3_x86_32(&key, sizeof(uint64_t), 0, &h);
    // inspired by https://github.com/google/leveldb/blob/main/util/bloom.cc
    const uint32_t delta = (h >> 17) | (h << 15);
    for (uint64_t i = 0; i < k; i++) {
        const uint32_t bitpos = h % m;
        bitset.set(bitpos);
        h += delta;
    }
}

bool BloomFilter::contains(uint64_t key) const {
    uint32_t h;
    MurmurHash3_x86_32(&key, sizeof(uint64_t), 0, &h);
    const uint32_t delta = (h >> 17) | (h << 15);
    for (uint64_t i = 0; i < k; i++) {
        const uint32_t bitpos = h % m;
        if (bitset[bitpos] == 0) {
            return false;
        }
        h += delta;
    }
    return true;
}

void BloomFilter::reset() {
    bitset.reset();
}
