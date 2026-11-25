// Name: Muhammad Musab Sial
// Reg No: 20255888
#ifndef AVLTREE_H
#define AVLTREE_H

#include "Structures.h"

struct AVLNode {
    int key;
    int value;
    int height;
    AVLNode* left;
    AVLNode* right;
};

class AVLTree {
private:
    AVLNode* root;
    int count;
    int rotations;
    bool showRotations;
    string treeName;

    int height(AVLNode* node) {
        if (node == NULL) {
            return 0;
        }
        return node->height;
    }

    int maxOf(int a, int b) {
        if (a > b) {
            return a;
        }
        return b;
    }

    void updateHeight(AVLNode* node) {
        node->height = 1 + maxOf(height(node->left), height(node->right));
    }

    int balanceFactor(AVLNode* node) {
        if (node == NULL) {
            return 0;
        }
        return height(node->left) - height(node->right);
    }

    AVLNode* rotateRight(AVLNode* y) {
        AVLNode* x = y->left;
        AVLNode* moved = x->right;
        x->right = y;
        y->left = moved;
        updateHeight(y);
        updateHeight(x);
        rotations++;
        if (showRotations) {
            cout << "    [" << treeName << "] right rotation at key " << y->key
                 << ", new subtree root " << x->key << endl;
        }
        return x;
    }

    AVLNode* rotateLeft(AVLNode* x) {
        AVLNode* y = x->right;
        AVLNode* moved = y->left;
        y->left = x;
        x->right = moved;
        updateHeight(x);
        updateHeight(y);
        rotations++;
        if (showRotations) {
            cout << "    [" << treeName << "] left rotation at key " << x->key
                 << ", new subtree root " << y->key << endl;
        }
        return y;
    }

    AVLNode* rebalance(AVLNode* node) {
        updateHeight(node);
        int balance = balanceFactor(node);
        if (balance > 1) {
            if (balanceFactor(node->left) < 0) {
                if (showRotations) {
                    cout << "    [" << treeName << "] LR case at key " << node->key << endl;
                }
                node->left = rotateLeft(node->left);
            } else if (showRotations) {
                cout << "    [" << treeName << "] LL case at key " << node->key << endl;
            }
            return rotateRight(node);
        }
        if (balance < -1) {
            if (balanceFactor(node->right) > 0) {
                if (showRotations) {
                    cout << "    [" << treeName << "] RL case at key " << node->key << endl;
                }
                node->right = rotateRight(node->right);
            } else if (showRotations) {
                cout << "    [" << treeName << "] RR case at key " << node->key << endl;
            }
            return rotateLeft(node);
        }
        return node;
    }

    AVLNode* insertNode(AVLNode* node, int key, int value, bool& inserted) {
        if (node == NULL) {
            AVLNode* fresh = new AVLNode;
            fresh->key = key;
            fresh->value = value;
            fresh->height = 1;
            fresh->left = NULL;
            fresh->right = NULL;
            inserted = true;
            return fresh;
        }
        if (key < node->key) {
            node->left = insertNode(node->left, key, value, inserted);
        } else if (key > node->key) {
            node->right = insertNode(node->right, key, value, inserted);
        } else {
            inserted = false;
            return node;
        }
        return rebalance(node);
    }

    AVLNode* minNode(AVLNode* node) {
        while (node->left != NULL) {
            node = node->left;
        }
        return node;
    }

    AVLNode* removeNode(AVLNode* node, int key, bool& removed) {
        if (node == NULL) {
            removed = false;
            return NULL;
        }
        if (key < node->key) {
            node->left = removeNode(node->left, key, removed);
        } else if (key > node->key) {
            node->right = removeNode(node->right, key, removed);
        } else {
            removed = true;
            if (node->left == NULL || node->right == NULL) {
                AVLNode* child = node->left;
                if (child == NULL) {
                    child = node->right;
                }
                delete node;
                return child;
            }
            AVLNode* successor = minNode(node->right);
            node->key = successor->key;
            node->value = successor->value;
            bool dummy = false;
            node->right = removeNode(node->right, successor->key, dummy);
        }
        return rebalance(node);
    }

    void destroy(AVLNode* node) {
        if (node == NULL) {
            return;
        }
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

    void inorder(AVLNode* node, int* keys, int& index) {
        if (node == NULL) {
            return;
        }
        inorder(node->left, keys, index);
        keys[index] = node->key;
        index++;
        inorder(node->right, keys, index);
    }

    void preorderPrint(AVLNode* node) {
        if (node == NULL) {
            return;
        }
        cout << node->key << " ";
        preorderPrint(node->left);
        preorderPrint(node->right);
    }

    void postorderPrint(AVLNode* node) {
        if (node == NULL) {
            return;
        }
        postorderPrint(node->left);
        postorderPrint(node->right);
        cout << node->key << " ";
    }

    void inorderPrint(AVLNode* node) {
        if (node == NULL) {
            return;
        }
        inorderPrint(node->left);
        cout << node->key << " ";
        inorderPrint(node->right);
    }

    void shapePrint(AVLNode* node, int depth) {
        if (node == NULL) {
            return;
        }
        shapePrint(node->right, depth + 1);
        cout << "    ";
        for (int i = 0; i < depth; i++) {
            cout << "      ";
        }
        cout << node->key << "(h" << node->height << ")" << endl;
        shapePrint(node->left, depth + 1);
    }

    bool checkNode(AVLNode* node, int low, int high) {
        if (node == NULL) {
            return true;
        }
        if (node->key <= low || node->key >= high) {
            return false;
        }
        int balance = balanceFactor(node);
        if (balance > 1 || balance < -1) {
            return false;
        }
        return checkNode(node->left, low, node->key) && checkNode(node->right, node->key, high);
    }

public:
    AVLTree(string name) {
        root = NULL;
        count = 0;
        rotations = 0;
        showRotations = false;
        treeName = name;
    }

    ~AVLTree() {
        destroy(root);
    }

    void clear() {
        destroy(root);
        root = NULL;
        count = 0;
    }

    void setShowRotations(bool show) {
        showRotations = show;
    }

    bool insert(int key, int value) {
        bool inserted = false;
        root = insertNode(root, key, value, inserted);
        if (inserted) {
            count++;
        }
        return inserted;
    }

    bool remove(int key) {
        bool removed = false;
        root = removeNode(root, key, removed);
        if (removed) {
            count--;
        }
        return removed;
    }

    int search(int key) {
        AVLNode* current = root;
        while (current != NULL) {
            if (key == current->key) {
                return current->value;
            }
            if (key < current->key) {
                current = current->left;
            } else {
                current = current->right;
            }
        }
        return -1;
    }

    int size() {
        return count;
    }

    int treeHeight() {
        return height(root);
    }

    int rotationCount() {
        return rotations;
    }

    bool isValidAVL() {
        return checkNode(root, -INF, INF);
    }

    int getKeysInOrder(int* keys) {
        int index = 0;
        inorder(root, keys, index);
        return index;
    }

    void printTraversals() {
        cout << "  In-order   : ";
        inorderPrint(root);
        cout << endl << "  Pre-order  : ";
        preorderPrint(root);
        cout << endl << "  Post-order : ";
        postorderPrint(root);
        cout << endl;
    }

    void printShape() {
        if (root == NULL) {
            cout << "    (empty tree)" << endl;
            return;
        }
        shapePrint(root, 0);
    }
};

#endif
