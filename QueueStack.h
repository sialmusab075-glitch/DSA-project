// Name: Muhammad Musab Sial
// Reg No: 20255888
#ifndef QUEUESTACK_H
#define QUEUESTACK_H

#include "Structures.h"

struct IntNode {
    int data;
    IntNode* next;
};

class IntQueue {
private:
    IntNode* front;
    IntNode* rear;
    int count;

public:
    IntQueue();
    ~IntQueue();
    bool isEmpty();
    int size();
    void enqueue(int value);
    int dequeue();
};

class IntStack {
private:
    IntNode* top;
    int count;

public:
    IntStack();
    ~IntStack();
    bool isEmpty();
    int size();
    void push(int value);
    int pop();
};

const int UNDO_ROAD = 0;
const int UNDO_CAPACITY = 1;
const int UNDO_SAFETY = 2;
const int UNDO_WINDOW = 3;

struct UndoRecord {
    int type;
    int a;
    int b;
    int oldValue;
    string description;
};

struct UndoNode {
    UndoRecord record;
    UndoNode* next;
};

class UndoStack {
private:
    UndoNode* top;
    int count;

public:
    UndoStack();
    ~UndoStack();
    bool isEmpty();
    int size();
    void push(UndoRecord record);
    bool pop(UndoRecord& record);
    void print();
};

#endif
