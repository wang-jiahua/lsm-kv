#include "skiplist.h"
#include <climits>
#include <iostream>
#include <random>

SkipList::SkipList() {
    head = std::make_shared<Node>(ULLONG_MAX, std::string(), maxLevel);
    level = 0;
    size = 0U;
}

SkipList::~SkipList() { reset(); }

int SkipList::getRandomLevel() {
    std::random_device seed;
    std::ranlux48 engine(seed());
    std::uniform_int_distribution<unsigned> distribution(0U, UINT32_MAX);
    int level = 0;
    while (distribution(engine) % 2U == 1U && level < maxLevel) {
        ++level;
    }
    return level;
}

void SkipList::put(uint64_t key, const std::string &s) {
    std::shared_ptr<Node> current = head;
    std::shared_ptr<Node> update[maxLevel + 1];

    for (int i = level; i >= 0; --i) {
        while (current->get_forward(i) != nullptr && current->get_forward(i)->get_key() < key) {
            current = current->get_forward(i);
        }
        update[i] = current;
    }
    current = current->get_forward(0U);
    // if key already exists
    if (current != nullptr && current->get_key() == key) {
        if (current->is_deleted()) {
            current->set_deleted(false);
        }
        size += s.length() - (current->get_value()).length();
        current->set_value(s);
        return;
    }
    // if key doesn't exist, insert a new node
    int randomLevel = getRandomLevel();
    auto node = std::make_shared<Node>(key, s, randomLevel);

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
    std::shared_ptr<Node> current = head;
    for (int i = maxLevel - 1; i >= 0; --i) {
        while (current->get_forward(i) != nullptr && current->get_forward(i)->get_key() < key) {
            current = current->get_forward(i);
        }
    }
    current = current->get_forward(0U);
    if (current != nullptr && current->get_key() == key && !(current->is_deleted())) {
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

void SkipList::scan(uint64_t lower, uint64_t upper, std::vector<std::pair<uint64_t, std::string>> &result) const {
    std::shared_ptr<Node> current = head;
    for (int i = maxLevel - 1; i >= 0; --i) {
        while (current->get_forward(i) != nullptr && current->get_forward(i)->get_key() < lower) {
            current = current->get_forward(i);
        }
    }
    current = current->get_forward(0U);
    while (current != nullptr && current->get_key() <= upper) {
        if (!(current->is_deleted())) {
            (void) result.emplace_back(current->get_key(), current->get_value());
        }
        current = current->get_forward(0U);
    }
}

bool SkipList::del(uint64_t key, bool in_index, bool in_immutable, bool not_in_immutable) {
    std::shared_ptr<Node> current = head;
    std::shared_ptr<Node> update[maxLevel + 1];

    for (int i = level; i >= 0; --i) {
        while (current->get_forward(i) != nullptr && current->get_forward(i)->get_key() < key) {
            current = current->get_forward(i);
        }
        update[i] = current;
    }
    current = current->get_forward(0U);

    // if key in memtable
    if (current != nullptr && current->get_key() == key) {
        // if key marked as deleted in memtable
        if (current->is_deleted()) {
            return false;
        }
        // if key not deleted
        for (int i = 0; i <= current->get_level() && update[i]->get_forward(i) == current; ++i) {
            update[i]->set_forward(i, current->get_forward(i));
        }
        while (level > 0 && head->get_forward(level) == nullptr) {
            --level;
        }
        size -= sizeof(key) * 2U + (current->get_value()).length() + 4U;
        return true;
    }
    // key not in memtable
    // if key in immutable memtable or in disk only, insert new node
    if (in_immutable || (not_in_immutable && in_index)) {
        int randomLevel = getRandomLevel();
        std::shared_ptr<Node> node = std::make_shared<Node>(key, "", randomLevel, true);

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
    return false;
}

void SkipList::reset() {
    if (head != nullptr) {
        std::shared_ptr<Node> current = head->get_forward(0);
        for (int i = 0; i < maxLevel; i++) {
            head->set_forward(i, nullptr);
        }
        while (current != nullptr) {
            std::shared_ptr<Node> next = current->get_forward(0);
            for (int i = 0; i < current->get_level(); i++) {
                current->set_forward(i, nullptr);
            }
            current = next;
        }
    }
    head = std::make_shared<Node>(ULLONG_MAX, std::string(), maxLevel);
    level = 0;
    size = 0U;
}

void SkipList::print() const {
    std::cout << "-------------------------------------------\n";
    std::shared_ptr<Node> current = head;
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
        current = current->get_forward(0U);
    }
    std::cout << "-------------------------------------------\n";
}

uint64_t SkipList::getSize() const { return size; }

Data SkipList::traverse() const {
    Data data;
    std::shared_ptr<Node> current = head->get_forward(0U);
    while (current != nullptr) {
        (void) data.emplace_back(DataNode(current->get_key(), current->get_value(), current->is_deleted()));
        current = current->get_forward(0U);
    }
    return data;
}
