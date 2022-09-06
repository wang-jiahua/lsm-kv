#include "skiplist.h"
#include <climits>
#include <iostream>
#include <random>

SkipList::SkipList() {
    head = new Node(ULLONG_MAX, std::string(), maxLevel);
    level = 0;
    size = 0;
}

SkipList::~SkipList() { reset(); }

int SkipList::getRandomLevel() {
    std::random_device seed;
    std::ranlux48 engine(seed());
    std::uniform_int_distribution<unsigned> distribution(0, UINT32_MAX);
    int level = 0;
    while (distribution(engine) % 2 == 1 && level < maxLevel) {
        ++level;
    }
    return level;
}

void SkipList::put(uint64_t key, const std::string &s) {
    Node *current = head;
    Node *update[maxLevel + 1];
    memset(update, 0, (maxLevel + 1) * sizeof(Node *));

    for (int i = level; i >= 0; --i) {
        while (current->get_forward(i) != nullptr && current->get_forward(i)->get_key() < key) {
            current = current->get_forward(i);
        }
        update[i] = current;
    }
    current = current->get_forward(0);
    // if key already exists
    if (current != nullptr && current->get_key() == key) {
        if (current->get_deleted()) {
            current->set_deleted(false);
        }
        size += s.length() - (current->get_value()).length();
        current->set_value(s);
        return;
    }
    // if key doesn't exist, insert a new node
    int randomLevel = getRandomLevel();
    Node *node = new Node(key, s, randomLevel);

    if (randomLevel > level) {
        for (int i = level + 1; i <= randomLevel; ++i) {
            update[i] = head;
        }
        level = randomLevel;
    }

    for (int i = 0; i <= randomLevel; ++i) {
        node->set_forward(i, update[i]->get_forward(i));
        update[i]->set_forward(i, node);
    }

    size += sizeof(uint64_t) + s.length() + sizeof(uint64_t) + sizeof(uint64_t); // key + value + key + offset
}

std::string SkipList::get(uint64_t key, bool &deleted, bool &found) const {
    Node *current = head;
    for (int i = maxLevel - 1; i >= 0; --i) {
        while (current->get_forward(i) != nullptr && current->get_forward(i)->get_key() < key) {
            current = current->get_forward(i);
        }
    }
    current = current->get_forward(0);
    if (current != nullptr && current->get_key() == key && !(current->get_deleted())) {
        deleted = false;
        found = true;
        return current->get_value();
    }
    // key not found
    if (current == nullptr || current->get_key() != key) {
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
        while (current->get_forward(i) != nullptr && current->get_forward(i)->get_key() < key) {
            current = current->get_forward(i);
        }
        update[i] = current;
    }
    current = current->get_forward(0);

    // if key in memtable
    if (current != nullptr && current->get_key() == key) {
        // if key marked as deleted in memtable
        if (current->get_deleted()) {
            return false;
        }
        // if key not deleted
        for (int i = 0; i <= current->get_level() && update[i]->get_forward(i) == current; ++i) {
            update[i]->set_forward(i, current->get_forward(i));
        }
        while (level > 0 && head->get_forward(level) == nullptr) {
            --level;
        }
        size -= sizeof(key) * 2 + (current->get_value()).length() + 4;
        delete current;
        return true;
    }
    // if key not in memtable or disk
    if (!inIndex) {
        return false;
    }
    // if key in disk only, insert new node
    int randomLevel = getRandomLevel();
    Node *node = new Node(key, std::string(), randomLevel, true);

    if (randomLevel > level) {
        for (int i = level + 1; i <= randomLevel; ++i) {
            update[i] = head;
        }
        level = randomLevel;
    }

    for (int i = 0; i <= randomLevel; ++i) {
        node->set_forward(i, update[i]->get_forward(i));
        update[i]->set_forward(i, node);
    }

    size += sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint64_t); // key + value(empty) + key + offset
    return true;
}

void SkipList::reset() {
    Node *current = head->get_forward(0);
    for (int i = 0; i < maxLevel; i++) {
        head->set_forward(i, nullptr);
    }
    while (current != nullptr) {
        Node *next = current->get_forward(0);
        for (int i = 0; i < current->get_level(); i++) {
            current->set_forward(i, nullptr);
        }
        delete current;
        current = next;
    }
    size = 0;
}

void SkipList::print() const {
    std::cout << "-------------------------------------------\n";
    Node *current = head;
    while (current != nullptr) {
        for (int j = 0; j <= current->get_level(); ++j) {
            std::cout << current->get_key() << "(";
            if (current->get_forward(j) != nullptr) {
                std::cout << current->get_forward(j)->get_key();
            } else {
                std::cout << "n";
            }
            std::cout << ")" << '\t';
        }
        std::cout << std::endl;
        current = current->get_forward(0);
    }
    std::cout << "-------------------------------------------\n";
}

uint64_t SkipList::getSize() const { return size; }

Data SkipList::traverse() const {
    Data data;
    Node *current = head->get_forward(0);
    while (current != nullptr) {
        data.emplace_back(DataNode(current->get_key(), current->get_value(), current->get_deleted()));
        current = current->get_forward(0);
    }
    return data;
}
