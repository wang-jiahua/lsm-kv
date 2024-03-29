#include <cstdint>
#include <iostream>
#include <string>
#include <filesystem>
#include <random>

#include "bench.h"

namespace fs = std::filesystem;

class ReadRand : public Bench {
private:
    const size_t nr_ops = 1024U * 1024U;
    const size_t bytes_per_op = 1U;
    const size_t nr_bytes = nr_ops * bytes_per_op;

    void regular_test() {
        std::random_device device;
        std::default_random_engine engine(device());
        std::uniform_int_distribution<uint64_t> uniform_dist(0U, nr_ops - 1U);
        uint64_t i;
        for (i = 0U; i < nr_ops; ++i) {
            store.put(i, std::string(bytes_per_op, 's'));
        }
        start();
        for (i = 0U; i < nr_ops; ++i) {
            (void) store.get(uniform_dist(engine));
        }
        stop();
        report(nr_ops, nr_bytes);
    }

public:
    explicit ReadRand(const std::string &dir, bool v = true)
            : Bench(dir, v) {
    }

    void start_test(void *args = nullptr) override {
        std::cout << "KVStore Random Read Bench" << std::endl;
        (void) fs::remove_all("data");
        regular_test();
    }
};

int main(int argc, char *argv[]) {
    bool verbose = (argc == 2 && std::string(argv[1]) == "-v");

    std::cout << "Usage: " << argv[0] << " [-v]" << std::endl;
    std::cout << "  -v: print extra info for failed tests [currently ";
    std::cout << (verbose ? "ON" : "OFF") << "]" << std::endl;
    std::cout << std::endl;
    (void) std::cout.flush();

    (void) fs::remove_all("data");

    ReadRand test("data", verbose);

    test.start_test();

    return 0;
}
