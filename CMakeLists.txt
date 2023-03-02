cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-Wall -pthread")

project(lsm-kv)

find_package(Threads REQUIRED)

add_executable(correctness skiplist.cc util/MurmurHash3.cc bloom.cc filter.cc index.cc disk.cc kvstore.cc test/correctness.cc)

add_executable(persistence skiplist.cc util/MurmurHash3.cc bloom.cc filter.cc index.cc disk.cc kvstore.cc test/persistence.cc)

add_executable(test_bloom skiplist.cc util/MurmurHash3.cc bloom.cc filter.cc index.cc disk.cc kvstore.cc test/test_bloom.cc)

add_executable(test_scan skiplist.cc util/MurmurHash3.cc bloom.cc filter.cc index.cc batch.h disk.cc kvstore.cc test/test_scan.cc)

add_executable(write_seq skiplist.cc util/MurmurHash3.cc bloom.cc filter.cc index.cc disk.cc kvstore.cc benchmark/write_seq.cc)

add_executable(write_rand skiplist.cc util/MurmurHash3.cc bloom.cc filter.cc index.cc disk.cc kvstore.cc benchmark/write_rand.cc)

add_executable(read_seq skiplist.cc util/MurmurHash3.cc bloom.cc filter.cc index.cc disk.cc kvstore.cc benchmark/read_seq.cc)

add_executable(read_rand skiplist.cc util/MurmurHash3.cc bloom.cc filter.cc index.cc disk.cc kvstore.cc benchmark/read_rand.cc)

target_link_libraries(correctness PRIVATE Threads::Threads)

enable_testing()

add_test(NAME correctness COMMAND correctness)

add_test(NAME persistence-prepare COMMAND persistence)

add_test(NAME persistence-recover COMMAND persistence -t)

add_test(NAME test_bloom COMMAND test_bloom)

add_test(NAME test_scan COMMAND test_scan)

add_test(NAME write_seq COMMAND write_seq)

add_test(NAME write_rand COMMAND write_rand)

add_test(NAME read_seq COMMAND read_seq)

add_test(NAME read_rand COMMAND read_rand)
