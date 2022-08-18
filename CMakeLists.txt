cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-Wall")

project(lsm-kv)

add_executable(correctness skiplist.cc index.cc disk.cc kvstore.cc correctness.cc)

add_executable(persistence skiplist.cc index.cc disk.cc kvstore.cc persistence.cc)

