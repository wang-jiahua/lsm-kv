#include <cstdint>
#include <iostream>
#include <string>

#include "test.h"

class CorrectnessTest : public Test {
private:
    const uint64_t SIMPLE_TEST_MAX = 512U;
    const uint64_t LARGE_TEST_MAX = 1024U * 2U;

    void regular_test(uint64_t max) {
        uint64_t i;

        // Test a single key
        EXPECT(not_found, store.get(1U));
        store.put(1U, "SE");
        EXPECT("SE", store.get(1U));
        EXPECT(true, store.del(1U));
        EXPECT(not_found, store.get(1U));
        EXPECT(false, store.del(1U));

        phase();

        // Test multiple key-value pairs
        for (i = 0U; i < max; ++i) {
            store.put(i, std::string(i + 1U, 's'));
            EXPECT(std::string(i + 1U, 's'), store.get(i));
        }
        phase();

        // Test after all insertions
        for (i = 0U; i < max; ++i) {
            EXPECT(std::string(i + 1U, 's'), store.get(i));
        }
        phase();

        // Test deletions
        for (i = 0U; i < max; i += 2U) {
            EXPECT(true, store.del(i));
        }
        for (i = 0U; i < max; ++i) {
            EXPECT((i & 1U) ? std::string(i + 1U, 's') : not_found, store.get(i));
        }
        for (i = 1U; i < max; ++i) {
            EXPECT(i & 1U, store.del(i));
        }
        phase();

        report();
    }

public:
    explicit CorrectnessTest(const std::string &dir, bool v = true)
            : Test(dir, v) {
    }

    void start_test(void *args = nullptr) override {
        std::cout << "KVStore Correctness Test" << std::endl;

        std::cout << "[Simple Test]" << std::endl;
        regular_test(SIMPLE_TEST_MAX);

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

    CorrectnessTest test("data/", verbose);

    test.start_test();

    return 0;
}
