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

    void dfsVisit(int u, int* component, int componentId, bool allowHazardous) {
        component[u] = componentId;
        for (int v = 0; v < n; v++) {
            if (component[v] == -1 && passable(u, v, allowHazardous)) {
                dfsVisit(v, component, componentId, allowHazardous);
            }
        }
    }

    void dfsOrderVisit(int u, bool* visited, int* order, int& count, bool allowHazardous) {
        visited[u] = true;
        order[count] = u;
        count++;
        for (int v = 0; v < n; v++) {
            if (!visited[v] && passable(u, v, allowHazardous)) {
                dfsOrderVisit(v, visited, order, count, allowHazardous);
            }
        }
    }

public:
    CampusGraph(int size) {
        n = size;
        adj = new Edge*[n];
        for (int i = 0; i < n; i++) {
            adj[i] = new Edge[n];
            for (int j = 0; j < n; j++) {
                adj[i][j].distance = 0;
                adj[i][j].capacity = 0;
                adj[i][j].status = ROAD_NONE;
                adj[i][j].load = 0;
            }
        }
    }

    ~CampusGraph() {
        for (int i = 0; i < n; i++) {
            delete[] adj[i];
        }
        delete[] adj;
    }

    int size() {
        return n;
    }

    bool validIndex(int u) {
        return u >= 0 && u < n;
    }

    bool hasRoad(int u, int v) {
        return validIndex(u) && validIndex(v) && adj[u][v].status != ROAD_NONE;
    }

    bool addRoad(int u, int v, int distance, int capacity, int status) {
        if (!validIndex(u) || !validIndex(v) || u == v || hasRoad(u, v)) {
            return false;
        }
        if (distance <= 0 || capacity <= 0 || status < ROAD_OPEN || status > ROAD_BLOCKED) {
            return false;
        }
        adj[u][v].distance = distance;
        adj[u][v].capacity = capacity;
        adj[u][v].status = status;
        adj[u][v].load = 0;
        adj[v][u] = adj[u][v];
        return true;
    }

    Edge getRoad(int u, int v) {
        return adj[u][v];
    }

    int getStatus(int u, int v) {
        return adj[u][v].status;
    }

    bool setStatus(int u, int v, int status) {
        if (!hasRoad(u, v) || status < ROAD_OPEN || status > ROAD_BLOCKED) {
            return false;
        }
        adj[u][v].status = status;
        adj[v][u].status = status;
        return true;
    }

    void addLoad(int u, int v, int people) {
        adj[u][v].load += people;
        adj[v][u].load += people;
    }

    void clearLoads() {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                adj[i][j].load = 0;
            }
        }
    }

    bool passable(int u, int v, bool allowHazardous) {
        int s = adj[u][v].status;
        if (s == ROAD_OPEN || s == ROAD_CONGESTED) {
            return true;
        }
        if (s == ROAD_HAZARDOUS && allowHazardous) {
            return true;
        }
        return false;
    }

    int travelCost(int u, int v, bool allowHazardous) {
        if (!passable(u, v, allowHazardous)) {
            return INF;
        }
        int s = adj[u][v].status;
        if (s == ROAD_CONGESTED) {
            return adj[u][v].distance * 2;
        }
        if (s == ROAD_HAZARDOUS) {
            return adj[u][v].distance * 4;
        }
        return adj[u][v].distance;
    }

    int roadCount() {
        int total = 0;
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                if (adj[i][j].status != ROAD_NONE) {
                    total++;
                }
            }
        }
        return total;
    }

    int bfs(int source, int* order, int* hops, bool allowHazardous) {
        for (int i = 0; i < n; i++) {
            hops[i] = -1;
        }
        IntQueue queue;
        int count = 0;
        hops[source] = 0;
        queue.enqueue(source);
        while (!queue.isEmpty()) {
            int u = queue.dequeue();
            order[count] = u;
            count++;
            for (int v = 0; v < n; v++) {
                if (hops[v] == -1 && passable(u, v, allowHazardous)) {
                    hops[v] = hops[u] + 1;
                    queue.enqueue(v);
                }
            }
        }
        return count;
    }

    int dfsOrder(int source, int* order, bool allowHazardous) {
        bool* visited = new bool[n];
        for (int i = 0; i < n; i++) {
            visited[i] = false;
        }
        int count = 0;
        dfsOrderVisit(source, visited, order, count, allowHazardous);
        delete[] visited;
        return count;
    }

    int components(int* component, bool allowHazardous) {
        for (int i = 0; i < n; i++) {
            component[i] = -1;
        }
        int total = 0;
        for (int i = 0; i < n; i++) {
            if (component[i] == -1) {
                dfsVisit(i, component, total, allowHazardous);
                total++;
            }
        }
        return total;
    }

    void dijkstra(int source, int* dist, int* parent, bool allowHazardous) {
        bool* done = new bool[n];
        for (int i = 0; i < n; i++) {
            dist[i] = INF;
            parent[i] = -1;
            done[i] = false;
        }
        dist[source] = 0;
        for (int step = 0; step < n; step++) {
            int u = -1;
            for (int i = 0; i < n; i++) {
                if (!done[i] && dist[i] != INF && (u == -1 || dist[i] < dist[u])) {
                    u = i;
                }
            }
            if (u == -1) {
                break;
            }
            done[u] = true;
            for (int v = 0; v < n; v++) {
                if (done[v]) {
                    continue;
                }
                int cost = travelCost(u, v, allowHazardous);
                if (cost != INF && dist[u] + cost < dist[v]) {
                    dist[v] = dist[u] + cost;
                    parent[v] = u;
                }
            }
        }
        delete[] done;
    }

    int buildPath(int* parent, int source, int destination, int* path) {
        IntStack stack;
        int current = destination;
        while (current != -1) {
            stack.push(current);
            if (current == source) {
                break;
            }
            current = parent[current];
        }
        if (current != source) {
            return 0;
        }
        int length = 0;
        while (!stack.isEmpty()) {
            path[length] = stack.pop();
            length++;
        }
        return length;
    }
};

#endif
