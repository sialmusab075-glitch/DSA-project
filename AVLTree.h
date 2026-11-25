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

    int height(AVLNode* node);
    int maxOf(int a, int b);
    void updateHeight(AVLNode* node);
    int balanceFactor(AVLNode* node);
    AVLNode* rotateRight(AVLNode* y);
    AVLNode* rotateLeft(AVLNode* x);
    AVLNode* rebalance(AVLNode* node);
    AVLNode* insertNode(AVLNode* node, int key, int value, bool& inserted);
    AVLNode* minNode(AVLNode* node);
    AVLNode* removeNode(AVLNode* node, int key, bool& removed);
    void destroy(AVLNode* node);
    void inorder(AVLNode* node, int* keys, int& index);
    void preorderPrint(AVLNode* node);
    void postorderPrint(AVLNode* node);
    void inorderPrint(AVLNode* node);
    void shapePrint(AVLNode* node, int depth);
    bool checkNode(AVLNode* node, int low, int high);

public:
    AVLTree(string name);
    ~AVLTree();
    void clear();
    void setShowRotations(bool show);
    bool insert(int key, int value);
    bool remove(int key);
    int search(int key);
    int size();
    int treeHeight();
    int rotationCount();
    bool isValidAVL();
    int getKeysInOrder(int* keys);
    void printTraversals();
    void printShape();
};

#endif
