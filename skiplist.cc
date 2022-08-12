#include "skiplist.h"
#include <climits>
#include <iostream>
#include <stdlib.h>

SkipList::SkipList() {
    head = new Node(ULLONG_MAX, std::string(), maxLevel);
    level = 0;
    size = 0;
}

SkipList::~SkipList() { reset(); }

int SkipList::randomLevel() {
    int level = 0;
    while (rand() % 2 && level < maxLevel) {
        ++level;
    }
    return level;
}

void SkipList::put(uint64_t key, const std::string &s) {
    Node *current = head;
    Node *update[maxLevel + 1];
    memset(update, 0, (maxLevel + 1) * sizeof(Node *));

    for (int i = level; i >= 0; --i) {
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    current = current->forward[0];
    // if key already exists
    if (current != nullptr && current->key == key) {
        if (current->deleted) {
            current->deleted = false;
        }
        size += s.length() - (current->value).length();
        current->value = s;
        return;
    }
    // if key doesn't exist, insert a new node
    int randomlevel = randomLevel();
    Node *node = new Node(key, s, randomlevel);

    if (randomlevel > level) {
        for (int i = level + 1; i <= randomlevel; ++i) {
            update[i] = head;
        }
        level = randomlevel;
    }

    for (int i = 0; i <= randomlevel; ++i) {
        node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = node;
    }

    // TODO: how to calculate the size
    size += sizeof(uint64_t) + s.length() + sizeof(uint64_t) + sizeof(uint64_t); // key + value + key + offset
}

std::string SkipList::get(uint64_t key, bool &deleted, bool &found) const {
    Node *current = head;
    for (int level = maxLevel - 1; level >= 0; --level) {
        while (current->forward[level] && current->forward[level]->key < key) {
            current = current->forward[level];
        }
    }
    current = current->forward[0];
    if (current && current->key == key && !(current->deleted)) {
        deleted = false;
        found = true;
        return current->value;
    }
    // key not found
    if (current == nullptr || current->key != key) {
        deleted = false;
        found = false;
        return {};
    }
    // key deleted
    deleted = true;
    found = true;
    return {};
}

bool SkipList::del(uint64_t key, bool inIndex) {
    Node *current = head;
    Node *update[maxLevel + 1];
    memset(update, 0, (maxLevel + 1) * sizeof(Node *));

    for (int i = level; i >= 0; --i) {
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    current = current->forward[0];

    // if key in memtable
    if (current != nullptr && current->key == key) {
        // if key marked as deleted in memtable
        if (current->deleted) {
            return false;
        }
        // if key not deleted
        for (int i = 0; i <= current->level && update[i]->forward[i] == current; ++i) {
            update[i]->forward[i] = current->forward[i];
        }
        while (level > 0 && head->forward[level] == nullptr) {
            --level;
        }
        size -= sizeof(key) * 2 + (current->value).length() + 4;
        delete current;
        return true;
    }
    // if key not in memtable or disk
    if (!inIndex) {
        return false;
    }
    // if key in disk only, insert new node
    int randomlevel = randomLevel();
    Node *node = new Node(key, std::string(), randomlevel, true);

    if (randomlevel > level) {
        for (int i = level + 1; i <= randomlevel; ++i) {
            update[i] = head;
        }
        level = randomlevel;
    }

    for (int i = 0; i <= randomlevel; ++i) {
        node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = node;
    }

    size += sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint64_t); // key + value(empty) + key + offset
    return true;
}

void SkipList::reset() {
    Node *current = head->forward[0];
    for (int i = 0; i < maxLevel; i++) {
        head->forward[i] = nullptr;
    }
    while (current != nullptr) {
        Node *next = current->forward[0];
        for (int i = 0; i < current->level; i++) {
            current->forward[i] = nullptr;
        }
        // TODO
//        delete current;
        current = next;
    }
    size = 0;
}

void SkipList::print() const {
    std::cout << "-------------------------------------------\n";
    Node *current = head;
    while (current) {
        for (int j = 0; j <= current->level; ++j) {
            std::cout << current->key << "(";
            if (current->forward[j]) {
                std::cout << current->forward[j]->key;
            } else {
                std::cout << "n";
            }
            std::cout << ")" << '\t';
        }
        std::cout << std::endl;
        current = current->forward[0];
    }
    std::cout << "-------------------------------------------\n";
}

uint64_t SkipList::getSize() const { return size; }

Data SkipList::traverse() const {
    Data data;
    Node *current = head->forward[0];
    while (current) {
        data.push_back(DataNode(current->key, current->value, current->deleted));
        current = current->forward[0];
    }
    return data;
}