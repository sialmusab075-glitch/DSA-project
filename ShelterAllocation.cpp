// Name: Muhammad Musab Sial
// Reg No: 20255888
#include "EvacuationPlanner.h"

int EvacuationPlanner::unservedAt(int i) {
    if (locs[i].type != TYPE_BUILDING || !locs[i].affected) {
        return 0;
    }
    return locs[i].population - locs[i].evacuated - locs[i].held;
}

void EvacuationPlanner::applyLoads(Allocation* a, int sign) {
    locs[a->shelter].occupancy += sign * a->people;
    locs[a->building].evacuated += sign * a->people;
    for (int i = 0; i + 1 < a->pathLength; i++) {
        graph->addLoad(a->path[i], a->path[i + 1], sign * a->people);
    }
}

Allocation* EvacuationPlanner::addAllocation(int building, int shelter, int people, int cost, int* p, int length, string reason) {
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
    a->pinned = false;
    a->reason = reason;
    a->next = NULL;
    if (allocTail == NULL) {
        allocHead = a;
        allocTail = a;
    } else {
        allocTail->next = a;
        allocTail = a;
    }
    applyLoads(a, 1);
    return a;
}

void EvacuationPlanner::releaseAllocation(Allocation* a) {
    applyLoads(a, -1);
}

void EvacuationPlanner::clearAllocations() {
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

int EvacuationPlanner::estimatedMinutes(Allocation* a) {
    int walk = (a->cost + WALK_SPEED - 1) / WALK_SPEED;
    int queueTime = 0;
    if (a->bottleneck > 0 && a->bottleneck != INF) {
        queueTime = (a->people + a->bottleneck - 1) / a->bottleneck;
    }
    return walk + queueTime;
}

string EvacuationPlanner::unservedReasonFor(int b) {
    int* d = new int[numLocs];
    int* p = new int[numLocs];
    graph->dijkstra(b, d, p, true, 0);
    bool anyReachable = false;
    bool freeReachable = false;
    for (int s = 0; s < numLocs; s++) {
        if (locs[s].type == TYPE_SHELTER && d[s] != INF) {
            anyReachable = true;
            if (locs[s].safe && locs[s].occupancy < locs[s].capacity) {
                freeReachable = true;
            }
        }
    }
    delete[] d;
    delete[] p;
    if (!anyReachable) {
        return "no open route to any shelter (disconnected zone)";
    }
    if (freeReachable) {
        return "shelters with space exist but every road to them is at capacity (" + toText(window) +
               "-minute window)";
    }
    return "every reachable shelter is full or unsafe";
}

void EvacuationPlanner::evacuateBuilding(int b, bool verbose) {
    int remaining = locs[b].population - locs[b].evacuated - locs[b].held;
    if (remaining <= 0) {
        return;
    }
    SortItem* candidates = new SortItem[numLocs];
    for (int pass = 0; pass < 2 && remaining > 0; pass++) {
        bool allowHazard = (pass == 1);
        while (remaining > 0) {
            graph->dijkstra(b, dist, parent, allowHazard, window);
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
            string skipped = "";
            int best = -1;
            for (int c = 0; c < count; c++) {
                int s = candidates[c].index;
                if (!locs[s].safe) {
                    skipped += locs[s].name + " (cost " + toText(dist[s]) + ") is UNSAFE; ";
                } else if (locs[s].occupancy >= locs[s].capacity) {
                    skipped += locs[s].name + " (cost " + toText(dist[s]) + ") is FULL; ";
                } else {
                    best = s;
                    break;
                }
            }
            if (best == -1) {
                break;
            }
            int length = graph->buildPath(parent, b, best, path);
            int freeSpace = locs[best].capacity - locs[best].occupancy;
            int limitFrom, limitTo;
            int roadRoom = pathResidual(path, length, limitFrom, limitTo);
            int take = remaining;
            if (freeSpace < take) {
                take = freeSpace;
            }
            if (roadRoom < take) {
                take = roadRoom;
            }
            string reason;
            if (skipped == "") {
                reason = "nearest feasible shelter";
            } else {
                reason = "skipped " + skipped + "chose " + locs[best].name + " (cost " + toText(dist[best]) + ")";
            }
            if (take == freeSpace && take < remaining) {
                reason += "; split, only " + toText(freeSpace) + " places left here";
            } else if (take == roadRoom && take < remaining) {
                reason += "; split, road " + locs[limitFrom].name + " - " + locs[limitTo].name +
                          " reaches its capacity limit";
            }
            if (allowHazard) {
                reason += "; WARNING: no safe road left, route uses a HAZARDOUS road";
            }
            addAllocation(b, best, take, dist[best], path, length, reason);
            if (verbose) {
                cout << "  " << locs[b].name << ": " << take << " people -> " << locs[best].name
                     << " [" << reason << "]" << endl;
            }
            remaining -= take;
        }
    }
    delete[] candidates;
    if (remaining > 0) {
        locs[b].unservedReason = unservedReasonFor(b);
        if (verbose) {
            cout << "  " << locs[b].name << ": " << remaining << " people UNSERVED ("
                 << locs[b].unservedReason << ")" << endl;
        }
    }
}

bool EvacuationPlanner::placePinned(int b, int s, int people, string& why) {
    if (!locs[s].safe) {
        why = locs[s].name + " is now unsafe";
        return false;
    }
    if (locs[s].capacity - locs[s].occupancy < people) {
        why = locs[s].name + " no longer has " + toText(people) + " free places";
        return false;
    }
    if (locs[b].population - locs[b].evacuated - locs[b].held < people) {
        why = locs[b].name + " no longer has that many people waiting";
        return false;
    }
    bool hazard = false;
    graph->dijkstra(b, dist, parent, false, window);
    if (dist[s] == INF) {
        graph->dijkstra(b, dist, parent, true, window);
        hazard = true;
    }
    if (dist[s] == INF) {
        why = "no route with spare road capacity";
        return false;
    }
    int length = graph->buildPath(parent, b, s, path);
    int limitFrom, limitTo;
    if (pathResidual(path, length, limitFrom, limitTo) < people) {
        why = "road " + locs[limitFrom].name + " - " + locs[limitTo].name + " is at capacity";
        return false;
    }
    string reason = "operator revision kept";
    if (hazard) {
        reason += "; WARNING route uses a HAZARDOUS road";
    }
    Allocation* a = addAllocation(b, s, people, dist[s], path, length, reason);
    a->pinned = true;
    return true;
}

void EvacuationPlanner::replanIfActive() {
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

void EvacuationPlanner::planEvacuation(bool verbose) {
    int pinCount = 0;
    for (Allocation* a = allocHead; a != NULL; a = a->next) {
        if (a->pinned) {
            pinCount++;
        }
    }
    int* pinBuilding = new int[pinCount + 1];
    int* pinShelter = new int[pinCount + 1];
    int* pinPeople = new int[pinCount + 1];
    int k = 0;
    for (Allocation* a = allocHead; a != NULL; a = a->next) {
        if (a->pinned) {
            pinBuilding[k] = a->building;
            pinShelter[k] = a->shelter;
            pinPeople[k] = a->people;
            k++;
        }
    }
    clearAllocations();
    for (int i = 0; i < pinCount; i++) {
        string why;
        if (placePinned(pinBuilding[i], pinShelter[i], pinPeople[i], why)) {
            if (verbose) {
                cout << "  Operator revision kept: " << pinPeople[i] << " people " << locs[pinBuilding[i]].name
                     << " -> " << locs[pinShelter[i]].name << endl;
            }
        } else {
            cout << "  Operator revision dropped (" << pinPeople[i] << " people " << locs[pinBuilding[i]].name
                 << " -> " << locs[pinShelter[i]].name << "): " << why << ". They are re-planned normally." << endl;
            addLog("Operator revision for " + locs[pinBuilding[i]].name + " dropped: " + why);
        }
    }
    delete[] pinBuilding;
    delete[] pinShelter;
    delete[] pinPeople;
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

bool EvacuationPlanner::cancelAllocation(int allocId) {
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
    locs[current->building].held += current->people;
    cout << "  Allocation " << allocId << " cancelled: " << current->people << " people from "
         << locs[current->building].name << " released from " << locs[current->shelter].name
         << ". They are held in place until the operator releases them." << endl;
    clock++;
    addLog("Allocation " + toText(allocId) + " cancelled, " + toText(current->people) + " people held at " +
           locs[current->building].name);
    delete[] current->path;
    delete current;
    return true;
}

bool EvacuationPlanner::releaseHeld(int buildingId) {
    int b = locationIndex.search(buildingId);
    if (b == -1 || locs[b].type != TYPE_BUILDING) {
        cout << "  Rejected: " << buildingId << " is not a building." << endl;
        return false;
    }
    if (locs[b].held == 0) {
        cout << "  No people are held at " << locs[b].name << "." << endl;
        return false;
    }
    cout << "  Released " << locs[b].held << " held people at " << locs[b].name << " for re-planning." << endl;
    locs[b].held = 0;
    clock++;
    addLog("Held people released at " + locs[b].name);
    replanIfActive();
    return true;
}

bool EvacuationPlanner::reviseAllocation(int allocId, int shelterId) {
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
    releaseAllocation(a);
    bool hazard = false;
    graph->dijkstra(a->building, dist, parent, false, window);
    if (dist[s] == INF) {
        graph->dijkstra(a->building, dist, parent, true, window);
        hazard = true;
    }
    if (dist[s] == INF) {
        applyLoads(a, 1);
        cout << "  Rejected: no route from " << locs[a->building].name << " to " << locs[s].name
             << " with spare road capacity." << endl;
        return false;
    }
    int length = graph->buildPath(parent, a->building, s, path);
    int limitFrom, limitTo;
    int roadRoom = pathResidual(path, length, limitFrom, limitTo);
    if (roadRoom < a->people) {
        applyLoads(a, 1);
        cout << "  Rejected: road " << locs[limitFrom].name << " - " << locs[limitTo].name << " can take only "
             << roadRoom << " more people, " << a->people << " needed." << endl;
        return false;
    }
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
    a->pinned = true;
    a->reason = "revised by operator (kept on re-plan while still feasible)";
    if (hazard) {
        a->reason += "; WARNING route uses a HAZARDOUS road";
    }
    applyLoads(a, 1);
    cout << "  Allocation " << allocId << " moved to " << locs[s].name << " (cost " << a->cost << ")." << endl;
    clock++;
    addLog("Allocation " + toText(allocId) + " revised to " + locs[s].name);
    return true;
}
