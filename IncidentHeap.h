// Name: Muhammad Musab Sial
// Reg No: 20255888
#ifndef INCIDENTHEAP_H
#define INCIDENTHEAP_H

#include "Structures.h"

bool higherPriority(Incident& a, Incident& b) {
    if (a.severity != b.severity) {
        return a.severity > b.severity;
    }
    if (a.population != b.population) {
        return a.population > b.population;
    }
    if (a.timeReceived != b.timeReceived) {
        return a.timeReceived < b.timeReceived;
    }
    return a.id < b.id;
}

class IncidentHeap {
private:
    int* heap;
    int* position;
    int size;
    int capacity;
    Incident* pool;

    bool before(int i, int j) {
        return higherPriority(pool[heap[i]], pool[heap[j]]);
    }

    void swapAt(int i, int j) {
        int temp = heap[i];
        heap[i] = heap[j];
        heap[j] = temp;
        position[heap[i]] = i;
        position[heap[j]] = j;
    }

    void siftUp(int i) {
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (before(i, parent)) {
                swapAt(i, parent);
                i = parent;
            } else {
                break;
            }
        }
    }

    void siftDown(int i) {
        while (true) {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            int best = i;
            if (left < size && before(left, best)) {
                best = left;
            }
            if (right < size && before(right, best)) {
                best = right;
            }
            if (best == i) {
                break;
            }
            swapAt(i, best);
            i = best;
        }
    }

public:
    IncidentHeap(Incident* incidentPool, int poolCapacity) {
        pool = incidentPool;
        capacity = poolCapacity;
        size = 0;
        heap = new int[capacity];
        position = new int[capacity];
        for (int i = 0; i < capacity; i++) {
            position[i] = -1;
        }
    }

    ~IncidentHeap() {
        delete[] heap;
        delete[] position;
    }

    bool isEmpty() {
        return size == 0;
    }

    int getSize() {
        return size;
    }

    bool contains(int slot) {
        return slot >= 0 && slot < capacity && position[slot] != -1;
    }

    bool insert(int slot) {
        if (size == capacity || contains(slot)) {
            return false;
        }
        heap[size] = slot;
        position[slot] = size;
        size++;
        siftUp(size - 1);
        return true;
    }

    int peek() {
        if (size == 0) {
            return -1;
        }
        return heap[0];
    }

    int extractMax() {
        if (size == 0) {
            return -1;
        }
        int top = heap[0];
        swapAt(0, size - 1);
        size--;
        position[top] = -1;
        if (size > 0) {
            siftDown(0);
        }
        return top;
    }

    bool remove(int slot) {
        if (!contains(slot)) {
            return false;
        }
        int i = position[slot];
        swapAt(i, size - 1);
        size--;
        position[slot] = -1;
        if (i < size) {
            siftUp(i);
            siftDown(i);
        }
        return true;
    }

    bool updatePriority(int slot) {
        if (!contains(slot)) {
            return false;
        }
        int i = position[slot];
        siftUp(i);
        siftDown(position[slot]);
        return true;
    }

    void rebuild() {
        for (int i = size / 2 - 1; i >= 0; i--) {
            siftDown(i);
        }
    }

    void buildFrom(int* slots, int n) {
        for (int i = 0; i < size; i++) {
            position[heap[i]] = -1;
        }
        size = 0;
        for (int i = 0; i < n && i < capacity; i++) {
            heap[size] = slots[i];
            position[slots[i]] = size;
            size++;
        }
        rebuild();
    }

    bool isValidHeap() {
        for (int i = 1; i < size; i++) {
            if (before(i, (i - 1) / 2)) {
                return false;
            }
        }
        return true;
    }

    int copyContents(int* out) {
        for (int i = 0; i < size; i++) {
            out[i] = heap[i];
        }
        return size;
    }
};

#endif
