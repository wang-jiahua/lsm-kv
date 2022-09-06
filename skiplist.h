#pragma once

#include "data.h"
#include <cstring>
#include <string>
#include <utility>

constexpr int maxLevel = 20;

class SkipList {
    struct Node {
    public:
        Node(uint64_t key, std::string s, int level, bool deleted = false)
                : key_(key), value_(std::move(s)), level_(level), deleted_(deleted) {
            forward_ = new Node *[level + 1];
            memset(forward_, 0, (level + 1) * sizeof(Node *));
        }

        ~Node() { delete[] forward_; }

        uint64_t get_key() const { return key_; }

        std::string get_value() const { return value_; }

        void set_value(std::string s) { value_ = std::move(s); }

        Node *get_forward(int i) const { return forward_[i]; }

        void set_forward(int i, Node *node) { forward_[i] = node; }

        int get_level() const { return level_; }

        bool get_deleted() const { return deleted_; }

        void set_deleted(bool deleted) { deleted_ = deleted; }

    private:
        uint64_t key_;
        std::string value_;
        Node **forward_;
        int level_;
        bool deleted_;
    };

private:
    Node *head;
    int level;
    uint64_t size;

    static int getRandomLevel();

public:
    SkipList();

    ~SkipList();

    void put(uint64_t key, const std::string &s);

    std::string get(uint64_t key, bool &deleted, bool &found) const;

    bool del(uint64_t key, bool inIndex);

    void reset();

    void print() const;

    [[nodiscard]] uint64_t getSize() const;

    [[nodiscard]] Data traverse() const;
};
