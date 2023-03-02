#include <cstdint>
#include <iostream>
#include <string>

#include "../bloom.h"
#include "test.h"

class BloomTest : public Test {
private:
    const uint64_t SIMPLE_TEST_MAX = 512U;
    const uint64_t MIDDLE_TEST_MAX = 1024U * 64U;
    const uint64_t LARGE_TEST_MAX = 1024U * 1024U * 8U;

    BloomFilter bloomFilter{};

    void regular_test(uint64_t max) {
        uint64_t i;

        // Test a single key
        EXPECT(false, bloomFilter.contains(1U));
        bloomFilter.add(1U);
        EXPECT(true, bloomFilter.contains(1U));
        bloomFilter.reset();
        EXPECT(false, bloomFilter.contains(1U));

        phase();

        // Test multiple additions
        for (i = 0U; i < max; ++i) {
            bloomFilter.add(i);
            EXPECT(true, bloomFilter.contains(i));
        }
        phase();

        // Test after all additions
        for (i = 0U; i < max; ++i) {
            EXPECT(true, bloomFilter.contains(i));
        }
        phase();

        bloomFilter.reset();

        // Test after reset
        for (i = 0U; i < max; i += 1U) {
            EXPECT(false, bloomFilter.contains(i));
        }
        phase();

        report();
    }

public:
    explicit BloomTest(const std::string &dir, bool v = true)
            : Test(dir, v) {
    }

    void start_test(void *args = nullptr) override {
        std::cout << "Bloom filter Test" << std::endl;

        std::cout << "[Simple Test]" << std::endl;
        regular_test(SIMPLE_TEST_MAX);

        std::cout << "[Middle Test]" << std::endl;
        regular_test(MIDDLE_TEST_MAX);

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

    BloomTest test("data", verbose);

    test.start_test();

    return 0;
}
