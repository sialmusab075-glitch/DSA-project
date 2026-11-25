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
    IntQueue() {
        front = NULL;
        rear = NULL;
        count = 0;
    }

    ~IntQueue() {
        while (!isEmpty()) {
            dequeue();
        }
    }

    bool isEmpty() {
        return front == NULL;
    }

    int size() {
        return count;
    }

    void enqueue(int value) {
        IntNode* node = new IntNode;
        node->data = value;
        node->next = NULL;
        if (rear == NULL) {
            front = node;
            rear = node;
        } else {
            rear->next = node;
            rear = node;
        }
        count++;
    }

    int dequeue() {
        if (isEmpty()) {
            return -1;
        }
        IntNode* temp = front;
        int value = temp->data;
        front = front->next;
        if (front == NULL) {
            rear = NULL;
        }
        delete temp;
        count--;
        return value;
    }
};

class IntStack {
private:
    IntNode* top;
    int count;

public:
    IntStack() {
        top = NULL;
        count = 0;
    }

    ~IntStack() {
        while (!isEmpty()) {
            pop();
        }
    }

    bool isEmpty() {
        return top == NULL;
    }

    int size() {
        return count;
    }

    void push(int value) {
        IntNode* node = new IntNode;
        node->data = value;
        node->next = top;
        top = node;
        count++;
    }

    int pop() {
        if (isEmpty()) {
            return -1;
        }
        IntNode* temp = top;
        int value = temp->data;
        top = top->next;
        delete temp;
        count--;
        return value;
    }
};

const int UNDO_ROAD = 0;
const int UNDO_CAPACITY = 1;
const int UNDO_SAFETY = 2;

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
    UndoStack() {
        top = NULL;
        count = 0;
    }

    ~UndoStack() {
        while (top != NULL) {
            UndoNode* temp = top;
            top = top->next;
            delete temp;
        }
    }

    bool isEmpty() {
        return top == NULL;
    }

    int size() {
        return count;
    }

    void push(UndoRecord record) {
        UndoNode* node = new UndoNode;
        node->record = record;
        node->next = top;
        top = node;
        count++;
    }

    bool pop(UndoRecord& record) {
        if (top == NULL) {
            return false;
        }
        UndoNode* temp = top;
        record = temp->record;
        top = top->next;
        delete temp;
        count--;
        return true;
    }

    void print() {
        if (top == NULL) {
            cout << "  (undo stack is empty)" << endl;
            return;
        }
        UndoNode* current = top;
        int position = 1;
        while (current != NULL) {
            cout << "  " << position << ". " << current->record.description;
            if (position == 1) {
                cout << "   <- top (undone first)";
            }
            cout << endl;
            current = current->next;
            position++;
        }
    }
};

#endif
