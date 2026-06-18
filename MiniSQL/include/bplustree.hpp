#ifndef MINISQL_BPLUSTREE_HPP
#define MINISQL_BPLUSTREE_HPP

#include <string>
#include <fstream>
#include "simple_array.hpp"
#include "schema.hpp"

// A small B+ tree for primary-key index.
// It stores one key -> one row number. Duplicate keys are rejected by the executor.
class index {
private:
    static const int MAX_KEYS = 4;

    struct Node {
        bool leaf;
        int n;
        std::string keys[MAX_KEYS + 1];
        int values[MAX_KEYS + 1];
        Node* children[MAX_KEYS + 2];
        Node* next;

        explicit Node(bool is_leaf) : leaf(is_leaf), n(0), next(nullptr) {
            for (int i = 0; i < MAX_KEYS + 2; ++i) {
                children[i] = nullptr;
            }
            for (int i = 0; i < MAX_KEYS + 1; ++i) {
                values[i] = -1;
            }
        }
    };

    Node* root_;
    ColumnType key_type_;

    int compare(const std::string& a, const std::string& b) const;
    void destroy(Node* node);
    bool insertRecursive(Node* node, const std::string& key, int value,
                         std::string& promoted_key, Node*& new_child);
    Node* findLeaf(const std::string& key) const;
    Node* leftmostLeaf() const;
    void collect(Node* leaf, SimpleArray<int>& output) const;

public:
    explicit index(ColumnType key_type = ColumnType::Int);
    ~index();

    index(const index&) = delete;
    index& operator=(const index&) = delete;

    void clear();
    bool insert(const std::string& key, int value);
    int findEqual(const std::string& key) const;
    void findLess(const std::string& key, SimpleArray<int>& output) const;
    void findGreater(const std::string& key, SimpleArray<int>& output) const;
    void dumpToFile(const std::string& path) const;
};

#endif
