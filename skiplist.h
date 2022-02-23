#pragma once

#include <cstring>
#include <string>

constexpr int maxLevel = 20;

class SkipList {
    struct Node {
        uint64_t key;
        std::string value;
        Node** forward;
        int level;

        Node(uint64_t _key, const std::string& s, int _level)
            : key(_key)
            , value(s)
            , level(_level)
        {
            forward = new Node*[level + 1];
            memset(forward, 0, (level + 1) * sizeof(Node*));
        }

        ~Node()
        {
            delete[] forward;
        }
    };

private:
    Node* head;
    int level;

    int randomLevel();

public:
    SkipList();

    ~SkipList();

    void put(uint64_t key, const std::string& s);

    std::string get(uint64_t key);

    bool del(uint64_t key);

    void reset();

    void print();
};