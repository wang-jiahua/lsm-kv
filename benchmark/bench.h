#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <chrono>

#include "../kvstore.h"

class Bench {
protected:
    void start() {
        time_begin = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    void stop() {
        time_end = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    void report(size_t nr_ops, size_t nr_bytes) const {
        double time_cost = static_cast<double>(time_end) - static_cast<double>(time_begin);
        std::cout << "cost " << time_cost << " ms" << std::endl;
        std::cout << (time_cost * 1000.0) / static_cast<double>(nr_ops) << " us/op" << std::endl;
        std::cout << (static_cast<double>(nr_bytes) / 1024.0 / 1024.0) / (time_cost / 1000.0) << " MB/s" << std::endl;
        (void) std::cout.flush();
    }

    class KVStore store;

    bool verbose;

public:
    explicit Bench(const std::string &dir, bool v = true) : store(dir), verbose(v) {}

    virtual void start_test(void *args = nullptr) {
        std::cout << "No test is implemented." << std::endl;
    }

private:
    std::time_t time_begin{0};
    std::time_t time_end{0};
};
