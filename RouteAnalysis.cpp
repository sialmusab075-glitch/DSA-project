// Name: Muhammad Musab Sial
// Reg No: 20255888
#include "EvacuationPlanner.h"

string EvacuationPlanner::pathToString(int* p, int length) {
    string text = "";
    for (int i = 0; i < length; i++) {
        if (i > 0) {
            text += " -> ";
        }
        text += locs[p[i]].name;
    }
    return text;
}

int EvacuationPlanner::pathBottleneck(int* p, int length) {
    int smallest = INF;
    for (int i = 0; i + 1 < length; i++) {
        int c = graph->getRoad(p[i], p[i + 1]).capacity;
        if (c < smallest) {
            smallest = c;
        }
    }
    return smallest;
}

bool EvacuationPlanner::pathUsesHazard(int* p, int length) {
    for (int i = 0; i + 1 < length; i++) {
        if (graph->getStatus(p[i], p[i + 1]) == ROAD_HAZARDOUS) {
            return true;
        }
    }
    return false;
}

bool EvacuationPlanner::shelterReachable(int i) {
    graph->dijkstra(i, dist, parent, true, 0);
    for (int s = 0; s < numLocs; s++) {
        if (locs[s].type == TYPE_SHELTER && locs[s].safe && dist[s] != INF) {
            return true;
        }
    }
    return false;
}

int EvacuationPlanner::pathResidual(int* p, int length, int& limitFrom, int& limitTo) {
    int smallest = INF;
    limitFrom = -1;
    limitTo = -1;
    for (int i = 0; i + 1 < length; i++) {
        int r = graph->residual(p[i], p[i + 1], window);
        if (r < smallest) {
            smallest = r;
            limitFrom = p[i];
            limitTo = p[i + 1];
        }
    }
    return smallest;
}

void EvacuationPlanner::traversalReport(int fromId) {
    int s = locationIndex.search(fromId);
    if (s == -1) {
        cout << "  Invalid location id " << fromId << endl;
        return;
    }
    int* order = new int[numLocs];
    int* hops = new int[numLocs];
    int count = graph->bfs(s, order, hops, false);
    cout << "  BFS from " << locs[s].name << " over safe roads (queue, level by level):" << endl << "   ";
    for (int i = 0; i < count; i++) {
        cout << " " << locs[order[i]].name << "(" << hops[order[i]] << ")";
    }
    cout << endl << "  Reachable shelters:";
    bool any = false;
    for (int i = 0; i < count; i++) {
        if (locs[order[i]].type == TYPE_SHELTER) {
            cout << " " << locs[order[i]].name << "[" << hops[order[i]] << " hops"
                 << (locs[order[i]].safe ? "" : ", UNSAFE") << "]";
            any = true;
        }
    }
    if (!any) {
        cout << " none";
    }
    cout << endl;
    count = graph->dfsOrder(s, order, false);
    cout << "  DFS (recursive) visit order:" << endl << "   ";
    for (int i = 0; i < count; i++) {
        cout << " " << locs[order[i]].name;
    }
    cout << endl;
    delete[] order;
    delete[] hops;
}

void EvacuationPlanner::connectivityReport() {
    int* component = new int[numLocs];
    int total = graph->components(component, true);
    cout << "  Connected zones found by recursive DFS (blocked roads removed): " << total << endl;
    for (int c = 0; c < total; c++) {
        bool hasShelter = false;
        int people = 0;
        for (int i = 0; i < numLocs; i++) {
            if (component[i] == c) {
                if (locs[i].type == TYPE_SHELTER && locs[i].safe) {
                    hasShelter = true;
                }
                people += locs[i].population;
            }
        }
        cout << "  Zone " << c + 1 << (hasShelter ? " [has safe shelter]" : " [DISCONNECTED - no safe shelter]")
             << " people " << people << ":";
        int shown = 0;
        for (int i = 0; i < numLocs; i++) {
            if (component[i] == c) {
                if (shown == 8) {
                    cout << endl << "        ";
                    shown = 0;
                }
                cout << " " << locs[i].name;
                shown++;
            }
        }
        cout << endl;
    }
    int* order = new int[numLocs];
    int* hops = new int[numLocs];
    cout << "  Shelters reachable from each affected building (BFS, safe roads only):" << endl;
    for (int b = 0; b < numLocs; b++) {
        if (locs[b].type != TYPE_BUILDING || !locs[b].affected) {
            continue;
        }
        int count = graph->bfs(b, order, hops, false);
        cout << "   " << setw(22) << locs[b].name << ":";
        int found = 0;
        for (int i = 0; i < count; i++) {
            if (locs[order[i]].type == TYPE_SHELTER) {
                cout << " " << locs[order[i]].name;
                found++;
            }
        }
        if (found == 0) {
            cout << " NONE - building is cut off";
        }
        cout << endl;
    }
    delete[] order;
    delete[] hops;
    delete[] component;
}

bool EvacuationPlanner::routeToShelter(int fromId) {
    int b = locationIndex.search(fromId);
    if (b == -1) {
        cout << "  Invalid location id " << fromId << endl;
        return false;
    }
    for (int pass = 0; pass < 2; pass++) {
        bool allowHazard = (pass == 1);
        graph->dijkstra(b, dist, parent, allowHazard, window);
        int best = -1;
        int nearest = -1;
        for (int s = 0; s < numLocs; s++) {
            if (locs[s].type != TYPE_SHELTER || dist[s] == INF) {
                continue;
            }
            if (nearest == -1 || dist[s] < dist[nearest]) {
                nearest = s;
            }
            if (locs[s].safe && locs[s].occupancy < locs[s].capacity && (best == -1 || dist[s] < dist[best])) {
                best = s;
            }
        }
        if (best != -1) {
            int length = graph->buildPath(parent, b, best, path);
            cout << "  Route " << locs[b].name << " -> " << locs[best].name << " (Dijkstra";
            cout << (allowHazard ? ", hazardous roads allowed" : ", safe roads only") << ")" << endl;
            cout << "    Path: " << pathToString(path, length) << endl;
            cout << "    Total cost: " << dist[best] << "   Bottleneck capacity: " << pathBottleneck(path, length)
                 << " people/min" << endl;
            if (nearest != best) {
                cout << "    Note: nearest shelter " << locs[nearest].name << " (cost " << dist[nearest] << ") is "
                     << (locs[nearest].safe ? "FULL" : "UNSAFE") << ", so the next feasible one was chosen." << endl;
            }
            if (allowHazard) {
                cout << "    WARNING: no safe route exists; this route crosses a hazardous road." << endl;
            }
            return true;
        }
    }
    cout << "  No feasible route from " << locs[b].name << ". Shelter by shelter:" << endl;
    int* open = new int[numLocs];
    int* openParent = new int[numLocs];
    graph->dijkstra(b, open, openParent, true, 0);
    for (int s = 0; s < numLocs; s++) {
        if (locs[s].type != TYPE_SHELTER) {
            continue;
        }
        cout << "    " << setw(18) << locs[s].name << ": ";
        if (open[s] == INF) {
            cout << "unreachable (all roads to it are blocked)";
        } else if (!locs[s].safe) {
            cout << "UNSAFE (cost " << open[s] << ")";
        } else if (locs[s].occupancy >= locs[s].capacity) {
            cout << "FULL " << locs[s].occupancy << "/" << locs[s].capacity << " (cost " << open[s] << ")";
        } else {
            cout << "has " << locs[s].capacity - locs[s].occupancy << " places, but every route to it is at road capacity";
        }
        cout << endl;
    }
    delete[] open;
    delete[] openParent;
    return false;
}

void EvacuationPlanner::routesForAffected() {
    for (int b = 0; b < numLocs; b++) {
        if (locs[b].type == TYPE_BUILDING && locs[b].affected) {
            routeToShelter(locs[b].id);
        }
    }
}

bool EvacuationPlanner::routeBetween(int fromId, int toId) {
    int a = locationIndex.search(fromId);
    int b = locationIndex.search(toId);
    if (a == -1 || b == -1) {
        cout << "  Invalid location id." << endl;
        return false;
    }
    graph->dijkstra(a, dist, parent, true, 0);
    if (dist[b] == INF) {
        cout << "  No route exists between " << locs[a].name << " and " << locs[b].name << "." << endl;
        return false;
    }
    int length = graph->buildPath(parent, a, b, path);
    cout << "  Path: " << pathToString(path, length) << "   cost " << dist[b]
         << (pathUsesHazard(path, length) ? "   (uses hazardous road)" : "") << endl;
    return true;
}
