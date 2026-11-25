// Name: Muhammad Musab Sial
// Reg No: 20255888
#include "AVLTree.h"

int AVLTree::height(AVLNode* node) {
    if (node == NULL) {
        return 0;
    }
    return node->height;
}

int AVLTree::maxOf(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

void AVLTree::updateHeight(AVLNode* node) {
    node->height = 1 + maxOf(height(node->left), height(node->right));
}

int AVLTree::balanceFactor(AVLNode* node) {
    if (node == NULL) {
        return 0;
    }
    return height(node->left) - height(node->right);
}

AVLNode* AVLTree::rotateRight(AVLNode* y) {
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

AVLNode* AVLTree::rotateLeft(AVLNode* x) {
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

AVLNode* AVLTree::rebalance(AVLNode* node) {
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

AVLNode* AVLTree::insertNode(AVLNode* node, int key, int value, bool& inserted) {
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

AVLNode* AVLTree::minNode(AVLNode* node) {
    while (node->left != NULL) {
        node = node->left;
    }
    return node;
}

AVLNode* AVLTree::removeNode(AVLNode* node, int key, bool& removed) {
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

void AVLTree::destroy(AVLNode* node) {
    if (node == NULL) {
        return;
    }
    destroy(node->left);
    destroy(node->right);
    delete node;
}

void AVLTree::inorder(AVLNode* node, int* keys, int& index) {
    if (node == NULL) {
        return;
    }
    inorder(node->left, keys, index);
    keys[index] = node->key;
    index++;
    inorder(node->right, keys, index);
}

void AVLTree::preorderPrint(AVLNode* node) {
    if (node == NULL) {
        return;
    }
    cout << node->key << " ";
    preorderPrint(node->left);
    preorderPrint(node->right);
}

void AVLTree::postorderPrint(AVLNode* node) {
    if (node == NULL) {
        return;
    }
    postorderPrint(node->left);
    postorderPrint(node->right);
    cout << node->key << " ";
}

void AVLTree::inorderPrint(AVLNode* node) {
    if (node == NULL) {
        return;
    }
    inorderPrint(node->left);
    cout << node->key << " ";
    inorderPrint(node->right);
}

void AVLTree::shapePrint(AVLNode* node, int depth) {
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

bool AVLTree::checkNode(AVLNode* node, int low, int high) {
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

AVLTree::AVLTree(string name) {
    root = NULL;
    count = 0;
    rotations = 0;
    showRotations = false;
    treeName = name;
}

AVLTree::~AVLTree() {
    destroy(root);
}

void AVLTree::clear() {
    destroy(root);
    root = NULL;
    count = 0;
}

void AVLTree::setShowRotations(bool show) {
    showRotations = show;
}

bool AVLTree::insert(int key, int value) {
    bool inserted = false;
    root = insertNode(root, key, value, inserted);
    if (inserted) {
        count++;
    }
    return inserted;
}

bool AVLTree::remove(int key) {
    bool removed = false;
    root = removeNode(root, key, removed);
    if (removed) {
        count--;
    }
    return removed;
}

int AVLTree::search(int key) {
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

int AVLTree::size() {
    return count;
}

int AVLTree::treeHeight() {
    return height(root);
}

int AVLTree::rotationCount() {
    return rotations;
}

bool AVLTree::isValidAVL() {
    return checkNode(root, -INF, INF);
}

int AVLTree::getKeysInOrder(int* keys) {
    int index = 0;
    inorder(root, keys, index);
    return index;
}

void AVLTree::printTraversals() {
    cout << "  In-order   : ";
    inorderPrint(root);
    cout << endl << "  Pre-order  : ";
    preorderPrint(root);
    cout << endl << "  Post-order : ";
    postorderPrint(root);
    cout << endl;
}

void AVLTree::printShape() {
    if (root == NULL) {
        cout << "    (empty tree)" << endl;
        return;
    }
    shapePrint(root, 0);
}
