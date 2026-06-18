#include "bplustree.hpp"
#include <cstdlib>

index::index(ColumnType key_type) : root_(nullptr), key_type_(key_type) {}

index::~index() {
    clear();
}

int index::compare(const std::string& a, const std::string& b) const {
    if (key_type_ == ColumnType::Int) {
        int ai = std::stoi(a);
        int bi = std::stoi(b);
        if (ai < bi) return -1;
        if (ai > bi) return 1;
        return 0;
    }
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

void index::destroy(Node* node) {
    if (node == nullptr) return;
    if (!node->leaf) {
        for (int i = 0; i <= node->n; ++i) {
            destroy(node->children[i]);
        }
    }
    delete node;
}

void index::clear() {
    destroy(root_);
    root_ = nullptr;
}

index::Node* index::findLeaf(const std::string& key) const {
    Node* cur = root_;
    while (cur != nullptr && !cur->leaf) {
        int i = 0;
        while (i < cur->n && compare(key, cur->keys[i]) >= 0) {
            ++i;
        }
        cur = cur->children[i];
    }
    return cur;
}

index::Node* index::leftmostLeaf() const {
    Node* cur = root_;
    while (cur != nullptr && !cur->leaf) {
        cur = cur->children[0];
    }
    return cur;
}

bool index::insert(const std::string& key, int value) {
    if (root_ == nullptr) {
        root_ = new Node(true);
        root_->keys[0] = key;
        root_->values[0] = value;
        root_->n = 1;
        return true;
    }

    std::string promoted;
    Node* new_child = nullptr;
    bool split = insertRecursive(root_, key, value, promoted, new_child);
    if (split) {
        Node* new_root = new Node(false);
        new_root->keys[0] = promoted;
        new_root->children[0] = root_;
        new_root->children[1] = new_child;
        new_root->n = 1;
        root_ = new_root;
    }
    return true;
}

bool index::insertRecursive(Node* node, const std::string& key, int value,
                            std::string& promoted_key, Node*& new_child) {
    if (node->leaf) {
        int pos = 0;
        while (pos < node->n && compare(node->keys[pos], key) < 0) {
            ++pos;
        }
        for (int i = node->n; i > pos; --i) {
            node->keys[i] = node->keys[i - 1];
            node->values[i] = node->values[i - 1];
        }
        node->keys[pos] = key;
        node->values[pos] = value;
        node->n++;

        if (node->n <= MAX_KEYS) {
            new_child = nullptr;
            return false;
        }

        Node* right = new Node(true);
        int total = node->n;
        int mid = total / 2;
        int right_count = total - mid;
        for (int i = 0; i < right_count; ++i) {
            right->keys[i] = node->keys[mid + i];
            right->values[i] = node->values[mid + i];
        }
        right->n = right_count;
        node->n = mid;
        right->next = node->next;
        node->next = right;
        promoted_key = right->keys[0];
        new_child = right;
        return true;
    }

    int child_pos = 0;
    while (child_pos < node->n && compare(key, node->keys[child_pos]) >= 0) {
        ++child_pos;
    }

    std::string child_promoted;
    Node* child_new = nullptr;
    bool child_split = insertRecursive(node->children[child_pos], key, value, child_promoted, child_new);
    if (!child_split) {
        new_child = nullptr;
        return false;
    }

    for (int i = node->n; i > child_pos; --i) {
        node->keys[i] = node->keys[i - 1];
    }
    for (int i = node->n + 1; i > child_pos + 1; --i) {
        node->children[i] = node->children[i - 1];
    }
    node->keys[child_pos] = child_promoted;
    node->children[child_pos + 1] = child_new;
    node->n++;

    if (node->n <= MAX_KEYS) {
        new_child = nullptr;
        return false;
    }

    Node* right = new Node(false);
    int total = node->n;
    int mid = total / 2;
    promoted_key = node->keys[mid];

    int right_key_count = total - mid - 1;
    for (int i = 0; i < right_key_count; ++i) {
        right->keys[i] = node->keys[mid + 1 + i];
    }
    for (int i = 0; i < right_key_count + 1; ++i) {
        right->children[i] = node->children[mid + 1 + i];
    }
    right->n = right_key_count;
    node->n = mid;
    new_child = right;
    return true;
}

int index::findEqual(const std::string& key) const {
    Node* leaf = findLeaf(key);
    if (leaf == nullptr) return -1;
    for (int i = 0; i < leaf->n; ++i) {
        if (compare(leaf->keys[i], key) == 0) {
            return leaf->values[i];
        }
    }
    return -1;
}

void index::findLess(const std::string& key, SimpleArray<int>& output) const {
    Node* leaf = leftmostLeaf();
    while (leaf != nullptr) {
        for (int i = 0; i < leaf->n; ++i) {
            if (compare(leaf->keys[i], key) < 0) {
                output.push_back(leaf->values[i]);
            } else {
                return;
            }
        }
        leaf = leaf->next;
    }
}

void index::findGreater(const std::string& key, SimpleArray<int>& output) const {
    Node* leaf = leftmostLeaf();
    while (leaf != nullptr) {
        for (int i = 0; i < leaf->n; ++i) {
            if (compare(leaf->keys[i], key) > 0) {
                output.push_back(leaf->values[i]);
            }
        }
        leaf = leaf->next;
    }
}

void index::dumpToFile(const std::string& path) const {
    std::ofstream out(path);
    Node* leaf = leftmostLeaf();
    while (leaf != nullptr) {
        for (int i = 0; i < leaf->n; ++i) {
            out << leaf->keys[i] << '\t' << leaf->values[i] << '\n';
        }
        leaf = leaf->next;
    }
}
