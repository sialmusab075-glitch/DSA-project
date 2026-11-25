// Name: Muhammad Musab Sial
// Reg No: 20255888
#include "EvacuationPlanner.h"

void EvacuationPlanner::reportAllocations() {
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

void EvacuationPlanner::reportUnserved() {
    int total = 0;
    for (int i = 0; i < numLocs; i++) {
        int left = unservedAt(i);
        if (left > 0) {
            cout << "  " << setw(22) << locs[i].name << setw(6) << left << " people  reason: "
                 << (locs[i].unservedReason == "" ? "not yet planned" : locs[i].unservedReason) << endl;
            total += left;
        }
    }
    int heldTotal = 0;
    for (int i = 0; i < numLocs; i++) {
        if (locs[i].held > 0) {
            cout << "  " << setw(22) << locs[i].name << setw(6) << locs[i].held
                 << " people  HELD in place by operator (allocation cancelled; release to re-plan)" << endl;
            heldTotal += locs[i].held;
        }
    }
    if (total == 0 && heldTotal == 0) {
        cout << "  Everyone in affected buildings has a shelter place." << endl;
    } else {
        cout << "  Total unserved: " << total << "   held by operator: " << heldTotal << endl;
    }
}

void EvacuationPlanner::reportShelterUtilization() {
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

void EvacuationPlanner::reportResources() {
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

void EvacuationPlanner::reportBottlenecks() {
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
    cout << "  Critical bottlenecks (insertion sort on load/capacity = minutes to clear; road limit = capacity x "
         << window << " min):" << endl;
    if (count == 0) {
        cout << "  (no evacuation traffic yet)" << endl;
    }
    for (int k = 0; k < count && k < 10; k++) {
        int i = fromList[items[k].index];
        int j = toList[items[k].index];
        Edge e = graph->getRoad(i, j);
        cout << "  " << setw(2) << k + 1 << ". " << setw(20) << locs[i].name << " - " << setw(20) << locs[j].name
             << "  load " << setw(5) << e.load << "  cap " << setw(3) << e.capacity << "/min  clear time "
             << setw(5) << fixed << setprecision(1) << items[k].key1 << " min  " << setw(5) << setprecision(0)
             << 100.0 * items[k].key1 / window << "% of limit  " << roadStatusName(e.status) << endl;
    }
    cout.unsetf(ios::fixed);
    cout << setprecision(6);
    delete[] items;
    delete[] fromList;
    delete[] toList;
}

void EvacuationPlanner::reportHighRisk() {
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

void EvacuationPlanner::reportIncidents() {
    applyAging();
    SortItem* items = new SortItem[incidentCap];
    int count = 0;
    for (int i = 0; i < incidentCap; i++) {
        if (incidents[i].used) {
            items[count].index = i;
            items[count].key1 = incidents[i].priority * 10000000.0 + incidents[i].severity * 1000000.0 + incidents[i].population;
            items[count].key2 = incidents[i].timeReceived;
            count++;
        }
    }
    quickSort(items, count);
    cout << "  Incidents ranked by priority rule (quick sort): aged priority, then true severity, then population, then earlier time" << endl;
    cout << "  " << setw(4) << "ID" << setw(18) << "Location" << setw(12) << "Type" << setw(5) << "Sev" << setw(5)
         << "Pri" << setw(7)
         << "Pop" << setw(6) << "Time" << setw(12) << "Status" << "  Resource / note" << endl;
    for (int k = 0; k < count; k++) {
        printIncidentRow(items[k].index);
    }
    delete[] items;
}

bool EvacuationPlanner::consistencyCheck() {
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
        if (locs[i].evacuated + locs[i].held > locs[i].population || locs[i].held < 0) {
            ok = false;
        }
    }
    int* expected = new int[numLocs * numLocs];
    for (int i = 0; i < numLocs * numLocs; i++) {
        expected[i] = 0;
    }
    for (Allocation* x = allocHead; x != NULL; x = x->next) {
        for (int i = 0; i + 1 < x->pathLength; i++) {
            expected[x->path[i] * numLocs + x->path[i + 1]] += x->people;
            expected[x->path[i + 1] * numLocs + x->path[i]] += x->people;
        }
    }
    for (int i = 0; i < numLocs; i++) {
        for (int j = 0; j < numLocs; j++) {
            if (i == j || !graph->hasRoad(i, j)) {
                continue;
            }
            if (graph->getRoad(i, j).load != expected[i * numLocs + j] || graph->residual(i, j, window) < 0) {
                ok = false;
            }
        }
    }
    delete[] expected;
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
    cout << "  Consistency check (occupancy and road loads = allocations, no shelter or road over capacity, no double assignment, heap/AVL valid): "
         << (ok ? "PASSED" : "FAILED") << endl;
    return ok;
}
