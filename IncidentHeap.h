// Name: Muhammad Musab Sial
// Reg No: 20255888
#ifndef INCIDENTHEAP_H
#define INCIDENTHEAP_H

#include "Structures.h"

bool higherPriority(Incident& a, Incident& b);

class IncidentHeap {
private:
    int* heap;
    int* position;
    int size;
    int capacity;
    Incident* pool;

    bool before(int i, int j);
    void swapAt(int i, int j);
    void siftUp(int i);
    void siftDown(int i);

public:
    IncidentHeap(Incident* incidentPool, int poolCapacity);
    ~IncidentHeap();
    bool isEmpty();
    int getSize();
    bool contains(int slot);
    bool insert(int slot);
    int peek();
    int extractMax();
    bool remove(int slot);
    bool updatePriority(int slot);
    void rebuild();
    void buildFrom(int* slots, int n);
    bool isValidHeap();
    int copyContents(int* out);
};

#endif
