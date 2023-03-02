#include <cstdint>
#include <iostream>
#include <string>
#include <filesystem>

#include "bench.h"

namespace fs = std::filesystem;

class WriteSeq : public Bench {
private:
    const size_t nr_ops = 1024U * 1024U;
    const size_t bytes_per_op = 1U;
    const size_t nr_bytes = nr_ops * bytes_per_op;

    void regular_test() {
        uint64_t i;
        for (i = 0U; i < nr_ops; ++i) {
            store.put(i, std::string(bytes_per_op, 's'));
        }
        start();
        for (i = 0U; i < nr_ops; ++i) {
            (void) store.get(i);
        }
        stop();
        report(nr_ops, nr_bytes);
    }

public:
    explicit WriteSeq(const std::string &dir, bool v = true)
            : Bench(dir, v) {
    }

    void start_test(void *args = nullptr) override {
        std::cout << "KVStore Sequential Read Bench" << std::endl;
        (void) fs::remove_all("data");
        regular_test();
//        (void) fs::remove_all("data");
    }
};

int main(int argc, char *argv[]) {
    bool verbose = (argc == 2 && std::string(argv[1]) == "-v");

    std::cout << "Usage: " << argv[0] << " [-v]" << std::endl;
    std::cout << "  -v: print extra info for failed tests [currently ";
    std::cout << (verbose ? "ON" : "OFF") << "]" << std::endl;
    std::cout << std::endl;
    (void) std::cout.flush();

    WriteSeq test("data", verbose);

    test.start_test();

    return 0;
}
