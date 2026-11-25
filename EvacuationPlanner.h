// Name: Muhammad Musab Sial
// Reg No: 20255888
#ifndef EVACUATIONPLANNER_H
#define EVACUATIONPLANNER_H

#include <fstream>
#include <iomanip>
#include <sstream>
#include "Structures.h"
#include "QueueStack.h"
#include "AVLTree.h"
#include "IncidentHeap.h"
#include "Sorting.h"
#include "CampusGraph.h"

struct LogNode {
    int time;
    string text;
    LogNode* next;
};

class EvacuationPlanner {
private:
    CampusGraph* graph;
    Location* locs;
    int numLocs;
    AVLTree locationIndex;
    Incident* incidents;
    int incidentCap;
    AVLTree incidentIndex;
    IncidentHeap* heap;
    Resource* resources;
    int numResources;
    Allocation* allocHead;
    Allocation* allocTail;
    int nextAllocId;
    bool planActive;
    UndoStack undoStack;
    LogNode* logHead;
    LogNode* logTail;
    int clock;
    int* dist;
    int* parent;
    int* path;

    string toText(int value) {
        stringstream ss;
        ss << value;
        return ss.str();
    }

    bool readToken(ifstream& in, string& token) {
        while (in >> token) {
            if (token[0] == '#') {
                string rest;
                getline(in, rest);
                continue;
            }
            return true;
        }
        return false;
    }

    bool readNumber(ifstream& in, int& value) {
        string token;
        if (!readToken(in, token)) {
            return false;
        }
        stringstream ss(token);
        ss >> value;
        if (ss.fail()) {
            return false;
        }
        char extra;
        if (ss >> extra) {
            return false;
        }
        return true;
    }

    bool expectHeader(ifstream& in, string name, int& count) {
        string token;
        if (!readToken(in, token) || token != name) {
            cout << "  Data error: expected section " << name << endl;
            return false;
        }
        if (!readNumber(in, count) || count < 0) {
            cout << "  Data error: bad count for section " << name << endl;
            return false;
        }
        return true;
    }

    void addLog(string text) {
        LogNode* node = new LogNode;
        node->time = clock;
        node->text = text;
        node->next = NULL;
        if (logTail == NULL) {
            logHead = node;
            logTail = node;
        } else {
            logTail->next = node;
            logTail = node;
        }
    }

    string pathToString(int* p, int length) {
        string text = "";
        for (int i = 0; i < length; i++) {
            if (i > 0) {
                text += " -> ";
            }
            text += locs[p[i]].name;
        }
        return text;
    }

    int pathBottleneck(int* p, int length) {
        int smallest = INF;
        for (int i = 0; i + 1 < length; i++) {
            int c = graph->getRoad(p[i], p[i + 1]).capacity;
            if (c < smallest) {
                smallest = c;
            }
        }
        return smallest;
    }

    bool pathUsesHazard(int* p, int length) {
        for (int i = 0; i + 1 < length; i++) {
            if (graph->getStatus(p[i], p[i + 1]) == ROAD_HAZARDOUS) {
                return true;
            }
        }
        return false;
    }

    int incidentScore(int locIndex) {
        int score = 0;
        for (int i = 0; i < incidentCap; i++) {
            if (incidents[i].used && incidents[i].location == locIndex &&
                (incidents[i].status == TASK_PENDING || incidents[i].status == TASK_DISPATCHED)) {
                score += incidents[i].severity * incidents[i].severity * 10;
            }
        }
        return score;
    }

    int unservedAt(int i) {
        if (locs[i].type != TYPE_BUILDING || !locs[i].affected) {
            return 0;
        }
        return locs[i].population - locs[i].evacuated;
    }

    bool shelterReachable(int i) {
        graph->dijkstra(i, dist, parent, true);
        for (int s = 0; s < numLocs; s++) {
            if (locs[s].type == TYPE_SHELTER && locs[s].safe && dist[s] != INF) {
                return true;
            }
        }
        return false;
    }

    Allocation* addAllocation(int building, int shelter, int people, int cost, int* p, int length, string reason) {
        Allocation* a = new Allocation;
        a->id = nextAllocId;
        nextAllocId++;
        a->building = building;
        a->shelter = shelter;
        a->people = people;
        a->cost = cost;
        a->path = new int[length];
        for (int i = 0; i < length; i++) {
            a->path[i] = p[i];
        }
        a->pathLength = length;
        a->bottleneck = pathBottleneck(p, length);
        a->hazardous = pathUsesHazard(p, length);
        a->reason = reason;
        a->next = NULL;
        if (allocTail == NULL) {
            allocHead = a;
            allocTail = a;
        } else {
            allocTail->next = a;
            allocTail = a;
        }
        locs[shelter].occupancy += people;
        locs[building].evacuated += people;
        for (int i = 0; i + 1 < length; i++) {
            graph->addLoad(p[i], p[i + 1], people);
        }
        return a;
    }

    void releaseAllocation(Allocation* a) {
        locs[a->shelter].occupancy -= a->people;
        locs[a->building].evacuated -= a->people;
        for (int i = 0; i + 1 < a->pathLength; i++) {
            graph->addLoad(a->path[i], a->path[i + 1], -a->people);
        }
    }

    void clearAllocations() {
        Allocation* current = allocHead;
        while (current != NULL) {
            Allocation* next = current->next;
            delete[] current->path;
            delete current;
            current = next;
        }
        allocHead = NULL;
        allocTail = NULL;
        nextAllocId = 1;
        for (int i = 0; i < numLocs; i++) {
            locs[i].occupancy = 0;
            locs[i].evacuated = 0;
            locs[i].unservedReason = "";
        }
        graph->clearLoads();
    }

    int estimatedMinutes(Allocation* a) {
        int walk = (a->cost + WALK_SPEED - 1) / WALK_SPEED;
        int queueTime = 0;
        if (a->bottleneck > 0 && a->bottleneck != INF) {
            queueTime = (a->people + a->bottleneck - 1) / a->bottleneck;
        }
        return walk + queueTime;
    }

    void evacuateBuilding(int b, bool verbose) {
        int remaining = locs[b].population - locs[b].evacuated;
        if (remaining <= 0) {
            return;
        }
        bool anyShelterSeen = false;
        string skipped = "";
        for (int pass = 0; pass < 2 && remaining > 0; pass++) {
            bool allowHazard = (pass == 1);
            graph->dijkstra(b, dist, parent, allowHazard);
            SortItem* candidates = new SortItem[numLocs];
            int count = 0;
            for (int s = 0; s < numLocs; s++) {
                if (locs[s].type == TYPE_SHELTER && dist[s] != INF) {
                    candidates[count].index = s;
                    candidates[count].key1 = -dist[s];
                    candidates[count].key2 = locs[s].id;
                    count++;
                }
            }
            insertionSort(candidates, count);
            if (count > 0) {
                anyShelterSeen = true;
            }
            for (int c = 0; c < count && remaining > 0; c++) {
                int s = candidates[c].index;
                if (!locs[s].safe) {
                    if (pass == 0) {
                        skipped += locs[s].name + " (cost " + toText(dist[s]) + ") is UNSAFE; ";
                    }
                    continue;
                }
                int freeSpace = locs[s].capacity - locs[s].occupancy;
                if (freeSpace <= 0) {
                    if (pass == 0) {
                        skipped += locs[s].name + " (cost " + toText(dist[s]) + ") is FULL; ";
                    }
                    continue;
                }
                int take = remaining;
                if (freeSpace < take) {
                    take = freeSpace;
                }
                int length = graph->buildPath(parent, b, s, path);
                string reason;
                if (skipped == "") {
                    reason = "nearest feasible shelter";
                } else {
                    reason = "skipped " + skipped + "chose " + locs[s].name + " (cost " + toText(dist[s]) + ")";
                }
                if (take < remaining) {
                    reason += "; split, only " + toText(freeSpace) + " places left here";
                }
                if (allowHazard) {
                    reason += "; WARNING: no safe road left, route uses a HAZARDOUS road";
                }
                addAllocation(b, s, take, dist[s], path, length, reason);
                if (verbose) {
                    cout << "  " << locs[b].name << ": " << take << " people -> " << locs[s].name
                         << " [" << reason << "]" << endl;
                }
                remaining -= take;
                if (take < freeSpace) {
                    break;
                }
                skipped += locs[s].name + " (cost " + toText(dist[s]) + ") is now FULL; ";
            }
            delete[] candidates;
        }
        if (remaining > 0) {
            if (!anyShelterSeen) {
                locs[b].unservedReason = "no open route to any shelter (disconnected zone)";
            } else {
                locs[b].unservedReason = "every reachable shelter is full or unsafe";
            }
            if (verbose) {
                cout << "  " << locs[b].name << ": " << remaining << " people UNSERVED ("
                     << locs[b].unservedReason << ")" << endl;
            }
        }
    }

    void replanIfActive() {
        if (!planActive) {
            return;
        }
        planEvacuation(false);
        int moved = 0;
        int left = 0;
        for (int i = 0; i < numLocs; i++) {
            moved += locs[i].type == TYPE_BUILDING && locs[i].affected ? locs[i].evacuated : 0;
            left += unservedAt(i);
        }
        cout << "  Evacuation plan recomputed: " << moved << " people allocated, " << left << " unserved." << endl;
    }

    int findIncidentSlot(int id) {
        return incidentIndex.search(id);
    }

    int findResource(int id) {
        for (int i = 0; i < numResources; i++) {
            if (resources[i].id == id) {
                return i;
            }
        }
        return -1;
    }

    int findBestResource(int slot, string& reason, int& bestCost) {
        Incident& inc = incidents[slot];
        int needed = requiredResource(inc.type);
        graph->dijkstra(inc.location, dist, parent, true);
        int best = -1;
        bestCost = INF;
        int exists = 0;
        int busy = 0;
        int unreachable = 0;
        for (int r = 0; r < numResources; r++) {
            if (resources[r].type != needed) {
                continue;
            }
            exists++;
            if (resources[r].busy) {
                busy++;
                continue;
            }
            int d = dist[resources[r].location];
            if (d == INF) {
                unreachable++;
                continue;
            }
            if (d < bestCost) {
                bestCost = d;
                best = r;
            }
        }
        if (best == -1) {
            if (exists == 0) {
                reason = "no " + resourceTypeName(needed) + " exists in the fleet";
            } else if (unreachable == 0) {
                reason = "insufficient resources: all " + toText(exists) + " " + resourceTypeName(needed) + " units are busy";
            } else {
                reason = "no free " + resourceTypeName(needed) + " can reach " + locs[inc.location].name +
                         " (" + toText(busy) + " busy, " + toText(unreachable) + " cut off by blocked roads)";
            }
        }
        return best;
    }

    void printIncidentRow(int slot) {
        Incident& inc = incidents[slot];
        cout << "  " << setw(4) << inc.id << setw(18) << locs[inc.location].name << setw(12)
             << incidentTypeName(inc.type) << setw(5) << inc.severity << setw(7) << inc.population
             << setw(6) << inc.timeReceived << setw(12) << taskStatusName(inc.status);
        if (inc.status == TASK_DISPATCHED && inc.resourceIndex >= 0) {
            cout << "  " << resources[inc.resourceIndex].name;
        } else if (inc.note != "") {
            cout << "  " << inc.note;
        }
        cout << endl;
    }

public:
    EvacuationPlanner() : locationIndex("LocationAVL"), incidentIndex("IncidentAVL") {
        graph = NULL;
        locs = NULL;
        numLocs = 0;
        incidentCap = 200;
        incidents = new Incident[incidentCap];
        for (int i = 0; i < incidentCap; i++) {
            incidents[i].used = false;
        }
        heap = new IncidentHeap(incidents, incidentCap);
        resources = NULL;
        numResources = 0;
        allocHead = NULL;
        allocTail = NULL;
        nextAllocId = 1;
        planActive = false;
        logHead = NULL;
        logTail = NULL;
        clock = 0;
        dist = NULL;
        parent = NULL;
        path = NULL;
    }

    ~EvacuationPlanner() {
        if (graph != NULL) {
            clearAllocations();
            delete graph;
        }
        delete[] locs;
        delete[] incidents;
        delete heap;
        delete[] resources;
        delete[] dist;
        delete[] parent;
        delete[] path;
        while (logHead != NULL) {
            LogNode* temp = logHead;
            logHead = logHead->next;
            delete temp;
        }
    }

    bool load(string filename) {
        ifstream in(filename.c_str());
        if (!in) {
            cout << "  Cannot open data file: " << filename << endl;
            return false;
        }
        int count;
        if (!expectHeader(in, "LOCATIONS", count)) {
            return false;
        }
        if (count < 2) {
            cout << "  Data error: need at least 2 locations" << endl;
            return false;
        }
        numLocs = count;
        locs = new Location[numLocs];
        graph = new CampusGraph(numLocs);
        dist = new int[numLocs];
        parent = new int[numLocs];
        path = new int[numLocs];
        for (int i = 0; i < numLocs; i++) {
            string typeText;
            int amount;
            int affected;
            if (!readNumber(in, locs[i].id) || !readToken(in, typeText) || !readToken(in, locs[i].name) ||
                !readNumber(in, amount) || !readNumber(in, affected)) {
                cout << "  Data error: location line " << i + 1 << " is incomplete" << endl;
                return false;
            }
            locs[i].type = parseLocationType(typeText);
            if (locs[i].type == -1 || locs[i].id <= 0 || amount < 0 || (affected != 0 && affected != 1)) {
                cout << "  Data error: invalid location " << locs[i].id << endl;
                return false;
            }
            locs[i].population = 0;
            locs[i].capacity = 0;
            if (locs[i].type == TYPE_BUILDING) {
                locs[i].population = amount;
            } else if (locs[i].type == TYPE_SHELTER) {
                locs[i].capacity = amount;
            }
            locs[i].affected = (affected == 1 && locs[i].type == TYPE_BUILDING);
            locs[i].occupancy = 0;
            locs[i].evacuated = 0;
            locs[i].safe = true;
            locs[i].unservedReason = "";
            if (!locationIndex.insert(locs[i].id, i)) {
                cout << "  Data error: duplicate location id " << locs[i].id << endl;
                return false;
            }
        }
        if (!expectHeader(in, "ROADS", count)) {
            return false;
        }
        for (int i = 0; i < count; i++) {
            int a, b, distance, capacity;
            string statusText;
            if (!readNumber(in, a) || !readNumber(in, b) || !readNumber(in, distance) ||
                !readNumber(in, capacity) || !readToken(in, statusText)) {
                cout << "  Data error: road line " << i + 1 << " is incomplete" << endl;
                return false;
            }
            int u = locationIndex.search(a);
            int v = locationIndex.search(b);
            int status = parseRoadStatus(statusText);
            if (u == -1 || v == -1 || !graph->addRoad(u, v, distance, capacity, status)) {
                cout << "  Data error: invalid road " << a << "-" << b << endl;
                return false;
            }
        }
        if (!expectHeader(in, "RESOURCES", count)) {
            return false;
        }
        numResources = count;
        resources = new Resource[numResources];
        for (int i = 0; i < numResources; i++) {
            string typeText;
            int locId;
            if (!readNumber(in, resources[i].id) || !readToken(in, typeText) || !readToken(in, resources[i].name) ||
                !readNumber(in, locId)) {
                cout << "  Data error: resource line " << i + 1 << " is incomplete" << endl;
                return false;
            }
            resources[i].type = parseResourceType(typeText);
            resources[i].location = locationIndex.search(locId);
            resources[i].busy = false;
            resources[i].incidentId = -1;
            if (resources[i].type == -1 || resources[i].location == -1 || findResource(resources[i].id) != i) {
                cout << "  Data error: invalid resource " << resources[i].id << endl;
                return false;
            }
        }
        if (!expectHeader(in, "INCIDENTS", count)) {
            return false;
        }
        for (int i = 0; i < count; i++) {
            int id, locId, severity, population, time;
            string typeText, description;
            if (!readNumber(in, id) || !readNumber(in, locId) || !readToken(in, typeText) ||
                !readNumber(in, severity) || !readNumber(in, population) || !readNumber(in, time) ||
                !readToken(in, description)) {
                cout << "  Data error: incident line " << i + 1 << " is incomplete" << endl;
                return false;
            }
            if (time > clock) {
                clock = time;
            }
            if (!registerIncident(id, locId, parseIncidentType(typeText), severity, population, description, time, false)) {
                cout << "  Data error: invalid incident " << id << endl;
                return false;
            }
        }
        addLog("Dataset loaded from " + filename);
        return true;
    }

    int locationIndexOf(int id) {
        return locationIndex.search(id);
    }

    void printSummary() {
        int buildings = 0, shelters = 0, gates = 0, junctions = 0, people = 0, affectedPeople = 0, space = 0;
        for (int i = 0; i < numLocs; i++) {
            if (locs[i].type == TYPE_BUILDING) {
                buildings++;
                people += locs[i].population;
                if (locs[i].affected) {
                    affectedPeople += locs[i].population;
                }
            } else if (locs[i].type == TYPE_SHELTER) {
                shelters++;
                space += locs[i].capacity;
            } else if (locs[i].type == TYPE_GATE) {
                gates++;
            } else {
                junctions++;
            }
        }
        int activeIncidents = 0;
        for (int i = 0; i < incidentCap; i++) {
            if (incidents[i].used) {
                activeIncidents++;
            }
        }
        cout << "  Locations: " << numLocs << " (" << buildings << " buildings, " << shelters << " shelters, "
             << gates << " gates, " << junctions << " junctions)" << endl;
        cout << "  Roads/corridors: " << graph->roadCount() << "   Rescue resources: " << numResources
             << "   Incidents on record: " << activeIncidents << endl;
        cout << "  People on campus: " << people << "   In affected buildings: " << affectedPeople
             << "   Total shelter capacity: " << space << endl;
    }

    void printLocations() {
        cout << "  " << setw(4) << "ID" << setw(22) << "Name" << setw(10) << "Type" << setw(8) << "People"
             << setw(10) << "Capacity" << setw(10) << "Affected" << setw(8) << "Safe" << endl;
        for (int i = 0; i < numLocs; i++) {
            cout << "  " << setw(4) << locs[i].id << setw(22) << locs[i].name << setw(10)
                 << locationTypeName(locs[i].type) << setw(8) << locs[i].population << setw(10)
                 << locs[i].capacity << setw(10) << (locs[i].affected ? "yes" : "-") << setw(8)
                 << (locs[i].type == TYPE_SHELTER ? (locs[i].safe ? "yes" : "NO") : "-") << endl;
        }
    }

    void printRoads() {
        cout << "  " << setw(18) << "From" << setw(22) << "To" << setw(8) << "Dist" << setw(6) << "Cap"
             << setw(11) << "Status" << setw(7) << "Cost" << endl;
        for (int i = 0; i < numLocs; i++) {
            for (int j = i + 1; j < numLocs; j++) {
                if (graph->hasRoad(i, j)) {
                    Edge e = graph->getRoad(i, j);
                    int cost = graph->travelCost(i, j, true);
                    cout << "  " << setw(18) << locs[i].name << setw(22) << locs[j].name << setw(8) << e.distance
                         << setw(6) << e.capacity << setw(11) << roadStatusName(e.status) << setw(7);
                    if (cost == INF) {
                        cout << "inf";
                    } else {
                        cout << cost;
                    }
                    cout << endl;
                }
            }
        }
    }

    void showLocationIndex() {
        cout << "  Location AVL index: " << locationIndex.size() << " keys, height " << locationIndex.treeHeight()
             << ", valid AVL: " << (locationIndex.isValidAVL() ? "yes" : "NO") << endl;
        locationIndex.printTraversals();
        cout << "  Tree shape (rotate page 90 degrees clockwise, root on the left):" << endl;
        locationIndex.printShape();
    }

    void showIncidentIndex() {
        cout << "  Incident AVL index: " << incidentIndex.size() << " keys, height " << incidentIndex.treeHeight()
             << ", rotations so far " << incidentIndex.rotationCount() << ", valid AVL: "
             << (incidentIndex.isValidAVL() ? "yes" : "NO") << endl;
        incidentIndex.printTraversals();
        incidentIndex.printShape();
    }

    void setShowRotations(bool show) {
        incidentIndex.setShowRotations(show);
        locationIndex.setShowRotations(show);
    }

    bool searchLocation(int id) {
        int i = locationIndex.search(id);
        if (i == -1) {
            cout << "  Location " << id << " not found in AVL index." << endl;
            return false;
        }
        cout << "  Found " << id << ": " << locs[i].name << " (" << locationTypeName(locs[i].type) << ")";
        if (locs[i].type == TYPE_BUILDING) {
            cout << ", people " << locs[i].population << ", evacuated " << locs[i].evacuated;
        }
        if (locs[i].type == TYPE_SHELTER) {
            cout << ", occupancy " << locs[i].occupancy << "/" << locs[i].capacity
                 << (locs[i].safe ? ", safe" : ", UNSAFE");
        }
        cout << endl;
        return true;
    }

    void traversalReport(int fromId) {
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

    void connectivityReport() {
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

    bool routeToShelter(int fromId) {
        int b = locationIndex.search(fromId);
        if (b == -1) {
            cout << "  Invalid location id " << fromId << endl;
            return false;
        }
        for (int pass = 0; pass < 2; pass++) {
            bool allowHazard = (pass == 1);
            graph->dijkstra(b, dist, parent, allowHazard);
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
        for (int s = 0; s < numLocs; s++) {
            if (locs[s].type != TYPE_SHELTER) {
                continue;
            }
            cout << "    " << setw(18) << locs[s].name << ": ";
            if (dist[s] == INF) {
                cout << "unreachable (all roads to it are blocked)";
            } else if (!locs[s].safe) {
                cout << "UNSAFE (cost " << dist[s] << ")";
            } else {
                cout << "FULL " << locs[s].occupancy << "/" << locs[s].capacity << " (cost " << dist[s] << ")";
            }
            cout << endl;
        }
        return false;
    }

    void routesForAffected() {
        for (int b = 0; b < numLocs; b++) {
            if (locs[b].type == TYPE_BUILDING && locs[b].affected) {
                routeToShelter(locs[b].id);
            }
        }
    }

    bool routeBetween(int fromId, int toId) {
        int a = locationIndex.search(fromId);
        int b = locationIndex.search(toId);
        if (a == -1 || b == -1) {
            cout << "  Invalid location id." << endl;
            return false;
        }
        graph->dijkstra(a, dist, parent, true);
        if (dist[b] == INF) {
            cout << "  No route exists between " << locs[a].name << " and " << locs[b].name << "." << endl;
            return false;
        }
        int length = graph->buildPath(parent, a, b, path);
        cout << "  Path: " << pathToString(path, length) << "   cost " << dist[b]
             << (pathUsesHazard(path, length) ? "   (uses hazardous road)" : "") << endl;
        return true;
    }

    void planEvacuation(bool verbose) {
        clearAllocations();
        SortItem* order = new SortItem[numLocs];
        int count = 0;
        for (int i = 0; i < numLocs; i++) {
            if (locs[i].type == TYPE_BUILDING && locs[i].affected && locs[i].population > 0) {
                order[count].index = i;
                order[count].key1 = incidentScore(i);
                order[count].key2 = -locs[i].population;
                count++;
            }
        }
        mergeSort(order, count);
        if (verbose) {
            cout << "  Evacuation order (merge sort: incident danger score, then population):" << endl << "   ";
            for (int i = 0; i < count; i++) {
                cout << " " << locs[order[i].index].name << "(" << order[i].key1 << ")";
            }
            cout << endl;
        }
        for (int i = 0; i < count; i++) {
            evacuateBuilding(order[i].index, verbose);
        }
        delete[] order;
        if (planActive) {
            addLog("Evacuation plan recomputed");
        } else {
            addLog("Evacuation plan computed");
        }
        planActive = true;
    }

    bool setRoadStatus(int fromId, int toId, int status, bool recordUndo) {
        int u = locationIndex.search(fromId);
        int v = locationIndex.search(toId);
        if (u == -1 || v == -1) {
            cout << "  Rejected: unknown location id." << endl;
            return false;
        }
        if (!graph->hasRoad(u, v)) {
            cout << "  Rejected: there is no road between " << locs[u].name << " and " << locs[v].name << "." << endl;
            return false;
        }
        if (status < ROAD_OPEN || status > ROAD_BLOCKED) {
            cout << "  Rejected: invalid road status." << endl;
            return false;
        }
        int old = graph->getStatus(u, v);
        if (old == status) {
            cout << "  No change: road is already " << roadStatusName(status) << "." << endl;
            return false;
        }
        graph->setStatus(u, v, status);
        string text = "Road " + locs[u].name + " - " + locs[v].name + ": " + roadStatusName(old) + " -> " +
                      roadStatusName(status);
        if (recordUndo) {
            UndoRecord r;
            r.type = UNDO_ROAD;
            r.a = u;
            r.b = v;
            r.oldValue = old;
            r.description = text;
            undoStack.push(r);
        }
        clock++;
        addLog(text);
        cout << "  " << text << endl;
        replanIfActive();
        return true;
    }

    bool setShelterCapacity(int id, int capacity, bool recordUndo) {
        int s = locationIndex.search(id);
        if (s == -1 || locs[s].type != TYPE_SHELTER) {
            cout << "  Rejected: " << id << " is not a shelter." << endl;
            return false;
        }
        if (capacity < 0) {
            cout << "  Rejected: capacity cannot be negative." << endl;
            return false;
        }
        int old = locs[s].capacity;
        if (old == capacity) {
            cout << "  No change: capacity is already " << capacity << "." << endl;
            return false;
        }
        locs[s].capacity = capacity;
        string text = "Shelter " + locs[s].name + " capacity: " + toText(old) + " -> " + toText(capacity);
        if (recordUndo) {
            UndoRecord r;
            r.type = UNDO_CAPACITY;
            r.a = s;
            r.b = -1;
            r.oldValue = old;
            r.description = text;
            undoStack.push(r);
        }
        clock++;
        addLog(text);
        cout << "  " << text << endl;
        if (planActive && locs[s].occupancy > capacity) {
            cout << "  Occupancy " << locs[s].occupancy << " now exceeds capacity, people are re-allocated." << endl;
        }
        replanIfActive();
        return true;
    }

    bool setShelterSafety(int id, bool safe, bool recordUndo) {
        int s = locationIndex.search(id);
        if (s == -1 || locs[s].type != TYPE_SHELTER) {
            cout << "  Rejected: " << id << " is not a shelter." << endl;
            return false;
        }
        if (locs[s].safe == safe) {
            cout << "  No change: shelter is already " << (safe ? "safe" : "unsafe") << "." << endl;
            return false;
        }
        locs[s].safe = safe;
        string text = "Shelter " + locs[s].name + " marked " + (safe ? "SAFE" : "UNSAFE");
        if (recordUndo) {
            UndoRecord r;
            r.type = UNDO_SAFETY;
            r.a = s;
            r.b = -1;
            r.oldValue = safe ? 0 : 1;
            r.description = text;
            undoStack.push(r);
        }
        clock++;
        addLog(text);
        cout << "  " << text << endl;
        replanIfActive();
        return true;
    }

    bool setAffected(int id, bool affected) {
        int b = locationIndex.search(id);
        if (b == -1 || locs[b].type != TYPE_BUILDING) {
            cout << "  Rejected: " << id << " is not a building." << endl;
            return false;
        }
        locs[b].affected = affected;
        addLog("Building " + locs[b].name + (affected ? " marked affected" : " cleared"));
        cout << "  " << locs[b].name << (affected ? " is now affected." : " is no longer affected.") << endl;
        replanIfActive();
        return true;
    }

    bool undoLast() {
        UndoRecord r;
        if (!undoStack.pop(r)) {
            cout << "  Nothing to undo." << endl;
            return false;
        }
        cout << "  Undoing: " << r.description << endl;
        if (r.type == UNDO_ROAD) {
            return setRoadStatus(locs[r.a].id, locs[r.b].id, r.oldValue, false);
        }
        if (r.type == UNDO_CAPACITY) {
            return setShelterCapacity(locs[r.a].id, r.oldValue, false);
        }
        return setShelterSafety(locs[r.a].id, r.oldValue == 1, false);
    }

    void printUndoStack() {
        cout << "  Undo stack (" << undoStack.size() << " records, LIFO):" << endl;
        undoStack.print();
    }

    bool registerIncident(int id, int locId, int type, int severity, int population, string description,
                          int time, bool announce) {
        if (id <= 0) {
            cout << "  Rejected: incident id must be positive." << endl;
            return false;
        }
        if (incidentIndex.search(id) != -1) {
            cout << "  Rejected: incident id " << id << " already exists." << endl;
            return false;
        }
        int loc = locationIndex.search(locId);
        if (loc == -1) {
            cout << "  Rejected: location " << locId << " does not exist." << endl;
            return false;
        }
        if (type < INC_FIRE || type > INC_MEDICAL) {
            cout << "  Rejected: unknown incident type." << endl;
            return false;
        }
        if (severity < 1 || severity > 5) {
            cout << "  Rejected: severity must be between 1 and 5." << endl;
            return false;
        }
        if (population < 0) {
            cout << "  Rejected: affected population cannot be negative." << endl;
            return false;
        }
        int slot = -1;
        for (int i = 0; i < incidentCap; i++) {
            if (!incidents[i].used) {
                slot = i;
                break;
            }
        }
        if (slot == -1) {
            cout << "  Rejected: incident storage is full." << endl;
            return false;
        }
        if (time < 0) {
            clock++;
            time = clock;
        }
        Incident& inc = incidents[slot];
        inc.id = id;
        inc.location = loc;
        inc.type = type;
        inc.severity = severity;
        inc.population = population;
        inc.timeReceived = time;
        inc.status = TASK_PENDING;
        inc.resourceIndex = -1;
        inc.used = true;
        inc.description = description;
        inc.note = "";
        incidentIndex.insert(id, slot);
        heap->insert(slot);
        addLog("Incident " + toText(id) + " registered at " + locs[loc].name + " severity " + toText(severity));
        if (announce) {
            cout << "  Incident " << id << " (" << incidentTypeName(type) << ", severity " << severity << ") registered at "
                 << locs[loc].name << ", time " << time << ". Pending tasks: " << heap->getSize() << endl;
        }
        if (planActive && locs[loc].type == TYPE_BUILDING && locs[loc].affected) {
            replanIfActive();
        }
        return true;
    }

    bool reviseIncident(int id, int severity, int population) {
        int slot = findIncidentSlot(id);
        if (slot == -1) {
            cout << "  Rejected: incident " << id << " not found." << endl;
            return false;
        }
        if (incidents[slot].status != TASK_PENDING) {
            cout << "  Rejected: only pending incidents can be revised (incident " << id << " is "
                 << taskStatusName(incidents[slot].status) << ")." << endl;
            return false;
        }
        if (severity < 1 || severity > 5 || population < 0) {
            cout << "  Rejected: severity must be 1-5 and population non-negative." << endl;
            return false;
        }
        cout << "  Incident " << id << " revised: severity " << incidents[slot].severity << " -> " << severity
             << ", population " << incidents[slot].population << " -> " << population << endl;
        incidents[slot].severity = severity;
        incidents[slot].population = population;
        heap->updatePriority(slot);
        addLog("Incident " + toText(id) + " priority revised");
        return true;
    }

    bool cancelIncident(int id) {
        int slot = findIncidentSlot(id);
        if (slot == -1) {
            cout << "  Rejected: incident " << id << " not found." << endl;
            return false;
        }
        Incident& inc = incidents[slot];
        if (inc.status == TASK_PENDING) {
            heap->remove(slot);
        } else if (inc.status == TASK_DISPATCHED) {
            resources[inc.resourceIndex].busy = false;
            resources[inc.resourceIndex].incidentId = -1;
            cout << "  " << resources[inc.resourceIndex].name << " released." << endl;
        } else {
            cout << "  Rejected: incident " << id << " is already " << taskStatusName(inc.status) << "." << endl;
            return false;
        }
        inc.status = TASK_CANCELLED;
        inc.note = "cancelled by operator";
        clock++;
        addLog("Incident " + toText(id) + " cancelled");
        cout << "  Incident " << id << " cancelled." << endl;
        return true;
    }

    bool dispatchNext() {
        if (heap->isEmpty()) {
            cout << "  No pending tasks in the priority queue." << endl;
            return false;
        }
        int* skipped = new int[incidentCap];
        int skipCount = 0;
        bool assigned = false;
        while (!heap->isEmpty()) {
            int slot = heap->extractMax();
            Incident& inc = incidents[slot];
            string reason;
            int cost;
            int r = findBestResource(slot, reason, cost);
            if (r == -1) {
                inc.note = reason;
                cout << "  Incident " << inc.id << " (sev " << inc.severity << ", " << locs[inc.location].name
                     << ") waits: " << reason << endl;
                skipped[skipCount] = slot;
                skipCount++;
                continue;
            }
            int length = graph->buildPath(parent, inc.location, resources[r].location, path);
            cout << "  DISPATCH incident " << inc.id << " (" << incidentTypeName(inc.type) << ", sev " << inc.severity
                 << ", pop " << inc.population << ") at " << locs[inc.location].name << " <- "
                 << resources[r].name << " from " << locs[resources[r].location].name << ", travel cost " << cost << endl;
            cout << "    Route: ";
            for (int i = length - 1; i >= 0; i--) {
                cout << locs[path[i]].name;
                if (i > 0) {
                    cout << " -> ";
                }
            }
            cout << endl;
            resources[r].busy = true;
            resources[r].incidentId = inc.id;
            inc.status = TASK_DISPATCHED;
            inc.resourceIndex = r;
            inc.note = "";
            clock++;
            addLog("Incident " + toText(inc.id) + " dispatched to " + resources[r].name);
            assigned = true;
            break;
        }
        for (int i = 0; i < skipCount; i++) {
            heap->insert(skipped[i]);
        }
        delete[] skipped;
        if (!assigned) {
            cout << "  No pending task can be served right now." << endl;
        }
        return assigned;
    }

    int dispatchAll() {
        int total = 0;
        while (!heap->isEmpty()) {
            if (!dispatchNext()) {
                break;
            }
            total++;
        }
        cout << "  Dispatched " << total << " task(s); still pending: " << heap->getSize() << endl;
        return total;
    }

    bool completeTask(int id) {
        int slot = findIncidentSlot(id);
        if (slot == -1) {
            cout << "  Rejected: incident " << id << " not found." << endl;
            return false;
        }
        Incident& inc = incidents[slot];
        if (inc.status != TASK_DISPATCHED) {
            cout << "  Rejected: incident " << id << " is " << taskStatusName(inc.status)
                 << ", only dispatched tasks can be completed." << endl;
            return false;
        }
        Resource& res = resources[inc.resourceIndex];
        res.busy = false;
        res.incidentId = -1;
        res.location = inc.location;
        inc.status = TASK_COMPLETED;
        clock++;
        addLog("Incident " + toText(id) + " completed by " + res.name);
        cout << "  Incident " << id << " completed. " << res.name << " is free again at " << locs[res.location].name << "."
             << endl;
        return true;
    }

    int purgeClosedIncidents() {
        int removed = 0;
        for (int i = 0; i < incidentCap; i++) {
            if (incidents[i].used && (incidents[i].status == TASK_COMPLETED || incidents[i].status == TASK_CANCELLED)) {
                cout << "  Deleting incident " << incidents[i].id << " from AVL index" << endl;
                incidentIndex.remove(incidents[i].id);
                incidents[i].used = false;
                removed++;
            }
        }
        addLog("Purged " + toText(removed) + " closed incident(s)");
        cout << "  Purged " << removed << " closed incident record(s). Index still valid AVL: "
             << (incidentIndex.isValidAVL() ? "yes" : "NO") << endl;
        return removed;
    }

    bool searchIncident(int id) {
        int slot = findIncidentSlot(id);
        if (slot == -1) {
            cout << "  Incident " << id << " not found." << endl;
            return false;
        }
        printIncidentRow(slot);
        return true;
    }

    bool cancelAllocation(int allocId) {
        Allocation* prev = NULL;
        Allocation* current = allocHead;
        while (current != NULL && current->id != allocId) {
            prev = current;
            current = current->next;
        }
        if (current == NULL) {
            cout << "  Rejected: allocation " << allocId << " not found." << endl;
            return false;
        }
        releaseAllocation(current);
        if (prev == NULL) {
            allocHead = current->next;
        } else {
            prev->next = current->next;
        }
        if (allocTail == current) {
            allocTail = prev;
        }
        locs[current->building].unservedReason = "allocation cancelled by operator";
        cout << "  Allocation " << allocId << " cancelled: " << current->people << " people from "
             << locs[current->building].name << " released from " << locs[current->shelter].name << endl;
        clock++;
        addLog("Allocation " + toText(allocId) + " cancelled");
        delete[] current->path;
        delete current;
        return true;
    }

    bool reviseAllocation(int allocId, int shelterId) {
        Allocation* a = allocHead;
        while (a != NULL && a->id != allocId) {
            a = a->next;
        }
        if (a == NULL) {
            cout << "  Rejected: allocation " << allocId << " not found." << endl;
            return false;
        }
        int s = locationIndex.search(shelterId);
        if (s == -1 || locs[s].type != TYPE_SHELTER) {
            cout << "  Rejected: " << shelterId << " is not a shelter." << endl;
            return false;
        }
        if (s == a->shelter) {
            cout << "  Rejected: allocation already uses " << locs[s].name << "." << endl;
            return false;
        }
        if (!locs[s].safe) {
            cout << "  Rejected: " << locs[s].name << " is unsafe." << endl;
            return false;
        }
        int freeSpace = locs[s].capacity - locs[s].occupancy;
        if (freeSpace < a->people) {
            cout << "  Rejected: " << locs[s].name << " has only " << freeSpace << " free places, " << a->people
                 << " needed. Capacity would be exceeded." << endl;
            return false;
        }
        graph->dijkstra(a->building, dist, parent, false);
        bool hazard = false;
        if (dist[s] == INF) {
            graph->dijkstra(a->building, dist, parent, true);
            hazard = true;
        }
        if (dist[s] == INF) {
            cout << "  Rejected: no route from " << locs[a->building].name << " to " << locs[s].name << "." << endl;
            return false;
        }
        int length = graph->buildPath(parent, a->building, s, path);
        releaseAllocation(a);
        delete[] a->path;
        a->path = new int[length];
        for (int i = 0; i < length; i++) {
            a->path[i] = path[i];
        }
        a->pathLength = length;
        a->shelter = s;
        a->cost = dist[s];
        a->bottleneck = pathBottleneck(path, length);
        a->hazardous = pathUsesHazard(path, length);
        a->reason = "revised by operator";
        if (hazard) {
            a->reason += "; WARNING route uses a HAZARDOUS road";
        }
        locs[s].occupancy += a->people;
        locs[a->building].evacuated += a->people;
        for (int i = 0; i + 1 < length; i++) {
            graph->addLoad(path[i], path[i + 1], a->people);
        }
        cout << "  Allocation " << allocId << " moved to " << locs[s].name << " (cost " << a->cost << ")." << endl;
        clock++;
        addLog("Allocation " + toText(allocId) + " revised to " + locs[s].name);
        return true;
    }

    void showHeap() {
        int* contents = new int[incidentCap];
        int n = heap->copyContents(contents);
        cout << "  Binary max-heap array (" << n << " pending), valid heap: " << (heap->isValidHeap() ? "yes" : "NO")
             << endl << "   ";
        for (int i = 0; i < n; i++) {
            cout << " [" << i << "]#" << incidents[contents[i]].id << "(s" << incidents[contents[i]].severity << ",p"
                 << incidents[contents[i]].population << ")";
            if (i % 5 == 4 && i + 1 < n) {
                cout << endl << "   ";
            }
        }
        cout << endl;
        if (n > 0) {
            cout << "  Next task to dispatch (root): incident " << incidents[heap->peek()].id << endl;
        }
        delete[] contents;
    }

    void reportAllocations() {
        cout << "  " << setw(3) << "ID" << setw(22) << "Building" << setw(18) << "Shelter" << setw(7) << "People"
             << setw(6) << "Cost" << setw(6) << "Min" << "  Route / reason" << endl;
        int total = 0;
        Allocation* a = allocHead;
        if (a == NULL) {
            cout << "  (no allocations)" << endl;
        }
        while (a != NULL) {
            cout << "  " << setw(3) << a->id << setw(22) << locs[a->building].name << setw(18) << locs[a->shelter].name
                 << setw(7) << a->people << setw(6) << a->cost << setw(6) << estimatedMinutes(a) << "  "
                 << pathToString(a->path, a->pathLength) << endl;
            cout << "  " << setw(62) << "" << "  why: " << a->reason << endl;
            total += a->people;
            a = a->next;
        }
        cout << "  Total people allocated: " << total << "   (Min = walking time at " << WALK_SPEED
             << " cost-units/min + queueing at the route bottleneck)" << endl;
    }

    void reportUnserved() {
        int total = 0;
        for (int i = 0; i < numLocs; i++) {
            int left = unservedAt(i);
            if (left > 0) {
                cout << "  " << setw(22) << locs[i].name << setw(6) << left << " people  reason: "
                     << (locs[i].unservedReason == "" ? "not yet planned" : locs[i].unservedReason) << endl;
                total += left;
            }
        }
        if (total == 0) {
            cout << "  Everyone in affected buildings has a shelter place." << endl;
        } else {
            cout << "  Total unserved: " << total << endl;
        }
    }

    void reportShelterUtilization() {
        SortItem* items = new SortItem[numLocs];
        int count = 0;
        for (int i = 0; i < numLocs; i++) {
            if (locs[i].type == TYPE_SHELTER) {
                items[count].index = i;
                items[count].key1 = locs[i].capacity == 0 ? 100.0 : 100.0 * locs[i].occupancy / locs[i].capacity;
                items[count].key2 = locs[i].id;
                count++;
            }
        }
        mergeSort(items, count);
        cout << "  Shelter utilization (sorted by merge sort, highest first):" << endl;
        cout << "  " << setw(18) << "Shelter" << setw(10) << "Occupied" << setw(10) << "Capacity" << setw(8) << "Used%"
             << setw(8) << "Safe" << endl;
        for (int k = 0; k < count; k++) {
            int i = items[k].index;
            cout << "  " << setw(18) << locs[i].name << setw(10) << locs[i].occupancy << setw(10) << locs[i].capacity
                 << setw(7) << fixed << setprecision(1) << items[k].key1 << "%" << setw(8)
                 << (locs[i].safe ? "yes" : "NO") << endl;
        }
        cout.unsetf(ios::fixed);
        cout << setprecision(6);
        delete[] items;
    }

    void reportResources() {
        cout << "  " << setw(4) << "ID" << setw(16) << "Name" << setw(13) << "Type" << setw(18) << "Location"
             << setw(11) << "Status" << endl;
        for (int r = 0; r < numResources; r++) {
            cout << "  " << setw(4) << resources[r].id << setw(16) << resources[r].name << setw(13)
                 << resourceTypeName(resources[r].type) << setw(18) << locs[resources[r].location].name << setw(11)
                 << (resources[r].busy ? "BUSY" : "AVAILABLE");
            if (resources[r].busy) {
                cout << "  -> incident " << resources[r].incidentId;
            }
            cout << endl;
        }
    }

    void reportBottlenecks() {
        int maxRoads = numLocs * (numLocs - 1) / 2;
        SortItem* items = new SortItem[maxRoads];
        int* fromList = new int[maxRoads];
        int* toList = new int[maxRoads];
        int count = 0;
        for (int i = 0; i < numLocs; i++) {
            for (int j = i + 1; j < numLocs; j++) {
                if (graph->hasRoad(i, j) && graph->getRoad(i, j).load > 0) {
                    Edge e = graph->getRoad(i, j);
                    fromList[count] = i;
                    toList[count] = j;
                    items[count].index = count;
                    items[count].key1 = (double)e.load / e.capacity;
                    items[count].key2 = e.capacity;
                    count++;
                }
            }
        }
        insertionSort(items, count);
        cout << "  Critical bottlenecks (sorted by insertion sort on load/capacity = minutes to clear):" << endl;
        if (count == 0) {
            cout << "  (no evacuation traffic yet)" << endl;
        }
        for (int k = 0; k < count && k < 10; k++) {
            int i = fromList[items[k].index];
            int j = toList[items[k].index];
            Edge e = graph->getRoad(i, j);
            cout << "  " << setw(2) << k + 1 << ". " << setw(20) << locs[i].name << " - " << setw(20) << locs[j].name
                 << "  load " << setw(5) << e.load << "  cap " << setw(3) << e.capacity << "/min  clear time "
                 << setw(5) << fixed << setprecision(1) << items[k].key1 << " min  " << roadStatusName(e.status) << endl;
        }
        cout.unsetf(ios::fixed);
        cout << setprecision(6);
        delete[] items;
        delete[] fromList;
        delete[] toList;
    }

    void reportHighRisk() {
        SortItem* items = new SortItem[numLocs];
        int count = 0;
        for (int i = 0; i < numLocs; i++) {
            int score = incidentScore(i);
            int left = unservedAt(i);
            int risk = score + left / 10;
            bool cutOff = false;
            if (locs[i].type == TYPE_BUILDING && (score > 0 || left > 0) && !shelterReachable(i)) {
                cutOff = true;
                risk += 50;
            }
            if (risk > 0) {
                items[count].index = i;
                items[count].key1 = risk;
                items[count].key2 = cutOff ? 0 : 1;
                count++;
            }
        }
        quickSort(items, count);
        cout << "  High-risk zones (quick sort, risk = 10*sum(severity^2) + unserved/10 + 50 if cut off):" << endl;
        for (int k = 0; k < count; k++) {
            int i = items[k].index;
            cout << "  " << setw(2) << k + 1 << ". " << setw(22) << locs[i].name << "  risk " << setw(5) << (int)items[k].key1
                 << "  incident score " << setw(4) << incidentScore(i) << "  unserved " << setw(4) << unservedAt(i)
                 << (items[k].key2 == 0 ? "  CUT OFF" : "") << endl;
        }
        if (count == 0) {
            cout << "  (no zone currently at risk)" << endl;
        }
        delete[] items;
    }

    void reportIncidents() {
        SortItem* items = new SortItem[incidentCap];
        int count = 0;
        for (int i = 0; i < incidentCap; i++) {
            if (incidents[i].used) {
                items[count].index = i;
                items[count].key1 = incidents[i].severity * 1000000.0 + incidents[i].population;
                items[count].key2 = incidents[i].timeReceived;
                count++;
            }
        }
        quickSort(items, count);
        cout << "  Incidents ranked by priority rule (quick sort): severity, then population, then earlier time" << endl;
        cout << "  " << setw(4) << "ID" << setw(18) << "Location" << setw(12) << "Type" << setw(5) << "Sev" << setw(7)
             << "Pop" << setw(6) << "Time" << setw(12) << "Status" << "  Resource / note" << endl;
        for (int k = 0; k < count; k++) {
            printIncidentRow(items[k].index);
        }
        delete[] items;
    }

    void printEventLog() {
        LogNode* current = logHead;
        while (current != NULL) {
            cout << "  t=" << setw(3) << current->time << "  " << current->text << endl;
            current = current->next;
        }
    }

    bool consistencyCheck() {
        bool ok = true;
        int* shelterSum = new int[numLocs];
        int* buildingSum = new int[numLocs];
        for (int i = 0; i < numLocs; i++) {
            shelterSum[i] = 0;
            buildingSum[i] = 0;
        }
        Allocation* a = allocHead;
        while (a != NULL) {
            shelterSum[a->shelter] += a->people;
            buildingSum[a->building] += a->people;
            a = a->next;
        }
        for (int i = 0; i < numLocs; i++) {
            if (shelterSum[i] != locs[i].occupancy || buildingSum[i] != locs[i].evacuated) {
                ok = false;
            }
            if (locs[i].type == TYPE_SHELTER && locs[i].occupancy > locs[i].capacity) {
                ok = false;
            }
            if (locs[i].evacuated > locs[i].population) {
                ok = false;
            }
        }
        for (int r = 0; r < numResources; r++) {
            int owners = 0;
            for (int i = 0; i < incidentCap; i++) {
                if (incidents[i].used && incidents[i].status == TASK_DISPATCHED && incidents[i].resourceIndex == r) {
                    owners++;
                }
            }
            if (owners > 1 || (owners == 1) != resources[r].busy) {
                ok = false;
            }
        }
        if (!heap->isValidHeap() || !incidentIndex.isValidAVL() || !locationIndex.isValidAVL()) {
            ok = false;
        }
        delete[] shelterSum;
        delete[] buildingSum;
        cout << "  Consistency check (occupancy = allocations, no over-capacity, no double assignment, heap/AVL valid): "
             << (ok ? "PASSED" : "FAILED") << endl;
        return ok;
    }

    int getClock() {
        return clock;
    }

    int allocationIdAt(int position) {
        Allocation* a = allocHead;
        for (int i = 0; i < position && a != NULL; i++) {
            a = a->next;
        }
        if (a == NULL) {
            return -1;
        }
        return a->id;
    }

    int allocationIdFor(int buildingId, int shelterId) {
        int b = locationIndex.search(buildingId);
        int s = locationIndex.search(shelterId);
        Allocation* a = allocHead;
        while (a != NULL) {
            if (a->building == b && (shelterId == -1 || a->shelter == s)) {
                return a->id;
            }
            a = a->next;
        }
        return -1;
    }
};

#endif
