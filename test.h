#pragma once

#include <cstdint>
#include <iostream>
#include <string>

#include "kvstore.h"

class Test {
protected:
    static const std::string not_found;

    uint64_t nr_tests;
    uint64_t nr_passed_tests;
    uint64_t nr_phases;
    uint64_t nr_passed_phases;

#define EXPECT(exp, got) expect<decltype(got)>((exp), (got), __FILE__, __LINE__)

    template<typename T>
    void expect(const T &exp, const T &got,
                const std::string &file, int line) {
        ++nr_tests;
        if (exp == got) {
            ++nr_passed_tests;
            return;
        }
        if (verbose) {
            std::cout << "TEST Error @" << file << ":" << line;
            std::cout << ", expected " << exp;
            std::cout << ", got " << got << std::endl;
        }
    }

    void phase() {
        // Report
        std::cout << "  Phase " << (nr_phases + 1) << ": ";
        std::cout << nr_passed_tests << "/" << nr_tests << " ";

        // Count
        ++nr_phases;
        if (nr_tests == nr_passed_tests) {
            ++nr_passed_phases;
            std::cout << "[PASS]" << std::endl;
        } else
            std::cout << "[FAIL]" << std::endl;

        std::cout.flush();

        // Reset
        nr_tests = 0;
        nr_passed_tests = 0;
    }

    void report() {
        std::cout << nr_passed_phases << "/" << nr_phases << " passed.";
        std::cout << std::endl;
        std::cout.flush();

        nr_phases = 0;
        nr_passed_phases = 0;
    }

    class KVStore store;

    bool verbose;

public:
    explicit Test(const std::string &dir, bool v = true)
            : store(dir), verbose(v) {
        nr_tests = 0;
        nr_passed_tests = 0;
        nr_phases = 0;
        nr_passed_phases = 0;
    }

    virtual void start_test(void *args = nullptr) {
        std::cout << "No test is implemented." << std::endl;
    }
};

const std::string Test::not_found;
