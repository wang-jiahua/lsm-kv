#include "skiplist.h"
#include <climits>
#include <iostream>
#include <stdlib.h>

SkipList::SkipList()
{
    head = new Node(ULLONG_MAX, std::string(), maxLevel);
    level = 0;
}

SkipList::~SkipList()
{
    reset();
}

int SkipList::randomLevel()
{
    int level = 0;
    while (rand() % 2 && level < maxLevel) {
        ++level;
    }
    return level;
}

void SkipList::put(uint64_t key, const std::string& s)
{
    Node* current = head;
    Node* update[maxLevel];
    memset(update, 0, (maxLevel + 1) * sizeof(Node*));
    for (int i = level; i >= 0; --i) {
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    current = current->forward[0];
    // if key already exists
    if (current && current->key == key) {
        current->value = s;
        return;
    }
    // else insert new node
    int randomlevel = randomLevel();
    Node* node = new Node(key, s, randomlevel);

    if (randomlevel > level) {
        for (int i = level + 1; i <= randomlevel; ++i) {
            head->forward[i] = node;
        }
        level = randomlevel;
    }

    for (int i = 0; i <= randomlevel; ++i) {
        if (update[i]) {
            node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = node;
        }
    }
}

std::string SkipList::get(uint64_t key)
{
    //  std::cout << "get " << key << std::endl;
    Node* current = head;
    for (int level = maxLevel - 1; level >= 0; --level) {
        while (current->forward[level] && current->forward[level]->key < key) {
            current = current->forward[level];
        }
    }
    current = current->forward[0];
    if (current && current->key == key) {
        return current->value;
    }
    return std::string();
}

bool SkipList::del(uint64_t key)
{
    Node* current = head;
    Node* update[maxLevel];

    for (int i = level; i >= 0; --i) {
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    current = current->forward[0];

    if (!current || current->key != key) {
        return false;
    }
    for (int i = 0; i <= current->level && update[i]->forward[i] == current; ++i) {
        update[i]->forward[i] = current->forward[i];
    }
    delete current;
    return true;
}

void SkipList::reset()
{
    Node* current = head->forward[0];
    while (current) {
        Node* next = head->forward[0];
        delete current;
        current = next;
    }
}

void SkipList::print()
{
    std::cout << "-------------------------------------------\n";
    Node* current = head;
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