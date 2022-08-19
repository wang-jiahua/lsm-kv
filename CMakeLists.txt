cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-Wall")

project(lsm-kv)

add_executable(correctness skiplist.cc MurmurHash3.cc bloom.cc filter.cc index.cc disk.cc kvstore.cc test/correctness.cc)

add_executable(persistence skiplist.cc MurmurHash3.cc bloom.cc filter.cc index.cc disk.cc kvstore.cc test/persistence.cc)

add_executable(test_bloom skiplist.cc MurmurHash3.cc bloom.cc filter.cc index.cc disk.cc kvstore.cc test/test_bloom.cc)
