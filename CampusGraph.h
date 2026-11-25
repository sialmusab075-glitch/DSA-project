// Name: Muhammad Musab Sial
// Reg No: 20255888
#ifndef CAMPUSGRAPH_H
#define CAMPUSGRAPH_H

#include "Structures.h"
#include "QueueStack.h"

class CampusGraph {
private:
    int n;
    Edge** adj;

    void dfsVisit(int u, int* component, int componentId, bool allowHazardous);
    void dfsOrderVisit(int u, bool* visited, int* order, int& count, bool allowHazardous);

public:
    CampusGraph(int size);
    ~CampusGraph();
    int size();
    bool validIndex(int u);
    bool hasRoad(int u, int v);
    bool addRoad(int u, int v, int distance, int capacity, int status);
    Edge getRoad(int u, int v);
    int getStatus(int u, int v);
    bool setStatus(int u, int v, int status);
    void addLoad(int u, int v, int people);
    int residual(int u, int v, int window);
    void clearLoads();
    bool passable(int u, int v, bool allowHazardous);
    int travelCost(int u, int v, bool allowHazardous);
    int roadCount();
    int bfs(int source, int* order, int* hops, bool allowHazardous);
    int dfsOrder(int source, int* order, bool allowHazardous);
    int components(int* component, bool allowHazardous);
    void dijkstra(int source, int* dist, int* parent, bool allowHazardous, int window);
    int buildPath(int* parent, int source, int destination, int* path);
};

#endif
