// Name: Muhammad Musab Sial
// Reg No: 20255888
#include "QueueStack.h"

IntQueue::IntQueue() {
    front = NULL;
    rear = NULL;
    count = 0;
}

IntQueue::~IntQueue() {
    while (!isEmpty()) {
        dequeue();
    }
}

bool IntQueue::isEmpty() {
    return front == NULL;
}

int IntQueue::size() {
    return count;
}

void IntQueue::enqueue(int value) {
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

int IntQueue::dequeue() {
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

IntStack::IntStack() {
    top = NULL;
    count = 0;
}

IntStack::~IntStack() {
    while (!isEmpty()) {
        pop();
    }
}

bool IntStack::isEmpty() {
    return top == NULL;
}

int IntStack::size() {
    return count;
}

void IntStack::push(int value) {
    IntNode* node = new IntNode;
    node->data = value;
    node->next = top;
    top = node;
    count++;
}

int IntStack::pop() {
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

UndoStack::UndoStack() {
    top = NULL;
    count = 0;
}

UndoStack::~UndoStack() {
    while (top != NULL) {
        UndoNode* temp = top;
        top = top->next;
        delete temp;
    }
}

bool UndoStack::isEmpty() {
    return top == NULL;
}

int UndoStack::size() {
    return count;
}

void UndoStack::push(UndoRecord record) {
    UndoNode* node = new UndoNode;
    node->record = record;
    node->next = top;
    top = node;
    count++;
}

bool UndoStack::pop(UndoRecord& record) {
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

void UndoStack::print() {
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
