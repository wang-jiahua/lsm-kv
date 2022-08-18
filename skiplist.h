#pragma once

#include "data.h"
#include <cstring>
#include <string>
#include <utility>

constexpr int maxLevel = 20;

class SkipList {
    struct Node {
        uint64_t key;
        std::string value;
        Node **forward;
        int level;
        bool deleted;

        Node(uint64_t _key, std::string s, int _level, bool _deleted = false)
                : key(_key), value(std::move(s)), level(_level), deleted(_deleted) {
            forward = new Node *[level + 1];
            memset(forward, 0, (level + 1) * sizeof(Node *));
        }

        ~Node() { delete[] forward; }
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