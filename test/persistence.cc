#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <filesystem>

#include "test.h"

namespace fs = std::filesystem;

class PersistenceTest : public Test {
private:
    const uint64_t TEST_MAX = 1024U * 2U;

    void prepare(uint64_t max) {
        uint64_t i;
        // Clean up
        store.reset();
        // Test multiple key-value pairs
        for (i = 0U; i < max; ++i) {
            store.put(i, std::string(i + 1, 's'));
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
        phase();

        // Prepare data for Test Mode
        for (i = 0U; i < max; ++i) {
            switch (i & 3U) {
                case 0U: {
                    EXPECT(not_found, store.get(i));
                    store.put(i, std::string(i + 1U, 't'));
                    EXPECT(std::string(i + 1U, 't'), store.get(i));
                    break;
                }
                case 1U: {
                    EXPECT(std::string(i + 1U, 's'), store.get(i));
                    store.put(i, std::string(i + 1U, 't'));
                    EXPECT(std::string(i + 1U, 't'), store.get(i));
                    break;
                }
                case 2U: {
                    EXPECT(not_found, store.get(i));
                    break;
                }
                case 3U: {
                    EXPECT(std::string(i + 1U, 's'), store.get(i));
                    break;
                }
                default: {
                    assert(0);
                }
            }
        }

        phase();

        report();

//        /**
//         * Write 10MB data to drain previous data out of memory.
//         */
//        for (i = 0U; i <= 10240U; ++i) {
//            store.put(max + i, std::string(1024U, 'x'));
//        }

        (void) std::cout.flush();
        // then exit as if crush
    }

    void test(uint64_t max) {
        uint64_t i;
        // Test data
        for (i = 0U; i < max; ++i) {
            switch (i & 3U) {
                case 0U:
                    EXPECT(std::string(i + 1U, 't'), store.get(i));
                    break;
                case 1U:
                    EXPECT(std::string(i + 1U, 't'), store.get(i));
                    break;
                case 2U:
                    EXPECT(not_found, store.get(i));
                    break;
                case 3U:
                    EXPECT(std::string(i + 1U, 's'), store.get(i));
                    break;
                default:
                    assert(0);
            }
        }

        phase();

        report();
    }

public:
    PersistenceTest(const std::string &dir, bool v = true)
            : Test(dir, v) {
    }

    void start_test(void *args = nullptr) override {
        bool testmode = (args != nullptr && *static_cast<bool *>(args));

        std::cout << "KVStore Persistence Test" << std::endl;

        if (testmode) {
            std::cout << "<<Test Mode>>" << std::endl;
            test(TEST_MAX);
            (void) fs::remove_all("data");
        } else {
            (void) fs::remove_all("data");
            std::cout << "<<Preparation Mode>>" << std::endl;
            prepare(TEST_MAX);
        }
    }
};

void usage(const char *prog, const char *verb, const char *mode) {
    std::cout << "Usage: " << prog << " [-t] [-v]" << std::endl;
    std::cout << "  -t: test mode for persistence test,"
                 " if -t is not given, the program only prepares data for test."
                 " [currently " << mode << "]" << std::endl;
    std::cout << "  -v: print extra info for failed tests [currently ";
    std::cout << verb << "]" << std::endl;
    std::cout << std::endl;
    std::cout << " NOTE: A normal usage is as follows:" << std::endl;
    std::cout << "    1. invoke `" << prog << "`;" << std::endl;
    std::cout << "    2. terminate (kill) the program when data is ready;";
    std::cout << std::endl;
    std::cout << "    3. invoke `" << prog << " -t ` to test." << std::endl;
    std::cout << std::endl;
    (void) std::cout.flush();
}

int main(int argc, char *argv[]) {
    bool verbose = false;
    bool testmode = false;

    if (argc == 2) {
        verbose = std::string(argv[1]) == "-v";
        testmode = std::string(argv[1]) == "-t";
    } else if (argc == 3) {
        verbose = std::string(argv[1]) == "-v" || std::string(argv[2]) == "-v";
        testmode = std::string(argv[1]) == "-t" || std::string(argv[2]) == "-t";
    } else if (argc > 3) {
        std::cerr << "Too many arguments." << std::endl;
        usage(argv[0], "OFF", "Preparation Mode");
        exit(-1);
    }
    usage(argv[0], verbose ? "ON" : "OFF", testmode ? "Test Mode" : "Preparation Mode");

    PersistenceTest test("data", verbose);

    test.start_test(static_cast<void *>(&testmode));

    return 0;
}
