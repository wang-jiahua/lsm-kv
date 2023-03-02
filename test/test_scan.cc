#include <cstdint>
#include <iostream>
#include <string>
#include <filesystem>

#include "test.h"

namespace fs = std::filesystem;

class ScanTest : public Test {
private:
    const uint64_t SIMPLE_TEST_MAX = 512U;
    const uint64_t LARGE_TEST_MAX = 1024U * 2U;

    void regular_test(uint64_t max) {
        uint64_t i;
        std::vector<std::pair<uint64_t, std::string>> result;

        // Test scan empty key-value store
        store.scan(0U, max, result);
        EXPECT(true, result.empty());
        phase();

        // Test scan after inserting all key-value pairs
        for (i = 0U; i < max; ++i) {
            store.put(i, std::string(i + 1U, 's'));
        }
        store.scan(0U, max, result);
        for (i = 0U; i < max; ++i) {
            EXPECT(std::string(i + 1U, 's'), result[i].second);
        }
        phase();

        // Test scan after deleting a half of key-value pairs
        for (i = 0U; i < max; i += 2U) {
            (void) store.del(i);
        }
        store.scan(0U, max, result);
        for (i = 0U; i < max / 2U; ++i) {
            EXPECT(std::string(2U * (i + 1U), 's'), result[i].second);
        }
        phase();

        // Test scan after deleting all key-value pairs
        for (i = 1U; i < max; i += 2U) {
            (void) store.del(i);
        }
        store.scan(0U, max, result);
        EXPECT(true, result.empty());
        phase();

        report();
    }

public:
    explicit ScanTest(const std::string &dir, bool v = true)
            : Test(dir, v) {
    }

    void start_test(void *args = nullptr) override {
        std::cout << "KVStore Scan Test" << std::endl;
        (void) fs::remove_all("data");

        std::cout << "[Simple Test]" << std::endl;
        regular_test(SIMPLE_TEST_MAX);
        (void) fs::remove_all("data");

        std::cout << "[Large Test]" << std::endl;
        regular_test(LARGE_TEST_MAX);
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

    ScanTest test("data", verbose);

    test.start_test();

    return 0;
}
