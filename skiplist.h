#pragma once

#include "data.h"

#include <cstring>
#include <string>
#include <utility>
#include <memory>
#include <vector>

constexpr int maxLevel = 20;

class SkipList {
    class Node {
    public:
        Node(uint64_t key, std::string s, int level, bool deleted = false)
                : key_(key), value_(std::move(s)), level_(level), deleted_(deleted) {
            forward_ = std::vector<std::shared_ptr<Node >>(level_ + 1);
        }

        ~Node() = default;

        [[nodiscard]] uint64_t get_key() const { return key_; }

        [[nodiscard]] std::string get_value() const { return value_; }

        void set_value(const std::string &s) { value_ = s; }

        [[nodiscard]] std::shared_ptr<Node> get_forward(size_t i) const { return forward_[i]; }

        void set_forward(size_t i, std::shared_ptr<Node> node) { forward_[i] = std::move(node); }

        [[nodiscard]] int get_level() const { return level_; }

        [[nodiscard]] bool is_deleted() const { return deleted_; }

        void set_deleted(bool deleted) { deleted_ = deleted; }

    private:
        uint64_t key_;
        std::string value_;
        std::vector<std::shared_ptr<Node>> forward_;
        int level_;
        bool deleted_;
    };

public:
    SkipList();

    ~SkipList();

    void put(uint64_t key, const std::string &s);

    std::string get(uint64_t key, bool &deleted, bool &found) const;

    void scan(uint64_t lower, uint64_t upper, std::vector<std::pair<uint64_t, std::string>> &result) const;

    bool del(uint64_t key, bool inIndex);

    void reset();

    void print() const;

    [[nodiscard]] uint64_t getSize() const;

    [[nodiscard]] Data traverse() const;

private:
    std::shared_ptr<Node> head;
    int level;
    uint64_t size;

    static int getRandomLevel();
};
