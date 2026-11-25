// Name: Muhammad Musab Sial
// Reg No: 20255888
#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <chrono>
#include "EvacuationPlanner.h"
using namespace std;

void heading(string title) {
    cout << endl << "==========================================================================" << endl;
    cout << title << endl;
    cout << "==========================================================================" << endl;
}

void subheading(string title) {
    cout << endl << "--- " << title << " ---" << endl;
}

int readInt(string prompt, int low, int high) {
    while (true) {
        cout << prompt;
        int value;
        if (cin >> value) {
            if (value >= low && value <= high) {
                return value;
            }
            cout << "  Please enter a number from " << low << " to " << high << "." << endl;
        } else {
            if (cin.eof()) {
                exit(0);
            }
            cin.clear();
            string junk;
            getline(cin, junk);
            cout << "  Invalid input, please enter a whole number." << endl;
        }
    }
}

string readWord(string prompt) {
    cout << prompt;
    string word;
    cin >> word;
    if (cin.eof()) {
        exit(0);
    }
    return word;
}

void allReports(EvacuationPlanner& planner) {
    subheading("Report: evacuation assignments");
    planner.reportAllocations();
    subheading("Report: unserved people");
    planner.reportUnserved();
    subheading("Report: shelter utilization");
    planner.reportShelterUtilization();
    subheading("Report: resource status");
    planner.reportResources();
    subheading("Report: critical bottlenecks");
    planner.reportBottlenecks();
    subheading("Report: ranked high-risk zones");
    planner.reportHighRisk();
    subheading("Report: incidents by priority");
    planner.reportIncidents();
}

void runDemo(string dataFile) {
    EvacuationPlanner planner;
    heading("SCENARIO 1: NORMAL OPERATION - load dataset, index, traverse, plan");
    if (!planner.load(dataFile)) {
        return;
    }
    planner.printSummary();
    subheading("Locations");
    planner.printLocations();
    subheading("Roads (cost = distance x1 open, x2 congested, x4 hazardous, blocked = inf)");
    planner.printRoads();
    subheading("AVL index of locations");
    planner.showLocationIndex();
    planner.searchLocation(16);
    planner.searchLocation(99);
    subheading("BFS and DFS from CS_Faculty");
    planner.traversalReport(5);
    subheading("Connectivity (disconnected zones)");
    planner.connectivityReport();
    subheading("Route and total cost from each affected building to its nearest feasible shelter");
    planner.routesForAffected();

    heading("SCENARIO 2: BLOCKED SHORTEST ROUTE, ROUTES RECOMPUTED");
    planner.routeToShelter(11);
    cout << "  Debris falls on the road Hostel_A - Football_Ground:" << endl;
    planner.setRoadStatus(11, 22, ROAD_BLOCKED, true);
    planner.routeToShelter(11);
    cout << "  Smoke makes Junction_East - Football_Ground hazardous:" << endl;
    planner.setRoadStatus(19, 22, ROAD_HAZARDOUS, true);
    planner.routeToShelter(11);

    heading("SCENARIO 3: EVACUATION PLAN, THEN FULL / UNSAFE NEAREST SHELTER");
    subheading("Initial evacuation plan");
    planner.planEvacuation(true);
    planner.reportAllocations();
    planner.reportShelterUtilization();
    planner.reportUnserved();
    planner.consistencyCheck();
    subheading("Nearest shelter becomes unsafe and another loses capacity");
    planner.routeToShelter(16);
    cout << "  Gas drifts over Assembly_North, it is declared unsafe:" << endl;
    planner.setShelterSafety(24, false, true);
    planner.routeToShelter(16);
    cout << "  Part of Cricket_Ground is flooded, capacity reduced:" << endl;
    planner.setShelterCapacity(21, 500, true);
    planner.reportAllocations();
    planner.reportShelterUtilization();
    planner.reportUnserved();
    planner.consistencyCheck();

    heading("SCENARIO 3b: ROAD CAPACITY LIMITS ROUTING");
    planner.reportBottlenecks();
    cout << "  Command wants everyone clear within 6 minutes instead of 10:" << endl;
    planner.setWindow(6, true);
    planner.reportAllocations();
    planner.reportBottlenecks();
    planner.reportUnserved();
    planner.routeToShelter(16);
    planner.consistencyCheck();
    planner.undoLast();
    planner.consistencyCheck();

    heading("SCENARIO 4: MULTIPLE INCIDENTS AND INSUFFICIENT RESOURCES");
    planner.reportIncidents();
    planner.showHeap();
    subheading("Dispatch the next task (highest priority)");
    planner.dispatchNext();
    subheading("Dispatch everything that can be served");
    planner.dispatchAll();
    planner.reportResources();
    planner.showHeap();
    subheading("Dispatching again while all units are busy");
    planner.dispatchNext();
    planner.consistencyCheck();

    heading("SCENARIO 5: DISCONNECTED LOCATION, THEN A CHANGE WHILE RUNNING");
    planner.connectivityReport();
    planner.routeToShelter(27);
    planner.routeBetween(15, 27);
    cout << "  Ground crew clears the Observatory - Guest_House road:" << endl;
    planner.setRoadStatus(27, 26, ROAD_OPEN, true);
    planner.routeBetween(15, 27);
    planner.routeToShelter(27);
    planner.reportUnserved();
    planner.consistencyCheck();

    heading("SCENARIO 6: TASKS COMPLETE, NEW INCIDENT ARRIVES, PRIORITY REVISED");
    planner.completeTask(12);
    planner.completeTask(1);
    planner.registerIncident(13, 4, INC_FIRE, 5, 200, "Library_fire", -1, true);
    planner.showHeap();
    cout << "  The sprain at Sports_Complex turns out to be a head injury:" << endl;
    planner.reviseIncident(11, 4, 2);
    planner.showHeap();
    planner.dispatchAll();
    planner.reportResources();
    planner.completeTask(5);
    planner.completeTask(4);
    planner.dispatchAll();
    planner.reportIncidents();
    planner.consistencyCheck();

    heading("SCENARIO 7: INVALID INPUT IS REJECTED");
    planner.registerIncident(3, 5, INC_FIRE, 3, 10, "Duplicate", -1, true);
    planner.registerIncident(50, 99, INC_FIRE, 3, 10, "Bad_location", -1, true);
    planner.registerIncident(51, 5, INC_FIRE, 9, 10, "Bad_severity", -1, true);
    planner.registerIncident(52, 5, INC_MEDICAL, 3, -4, "Bad_population", -1, true);
    planner.setShelterCapacity(23, -5, true);
    planner.setShelterCapacity(5, 100, true);
    planner.setRoadStatus(3, 9, ROAD_BLOCKED, true);
    planner.setRoadStatus(1, 3, 7, true);
    planner.completeTask(3);
    planner.completeTask(12);
    planner.cancelAllocation(999);
    planner.reviseIncident(2, 3, 300);
    int someAlloc = planner.allocationIdFor(9, -1);
    planner.reviseAllocation(someAlloc, 25);
    planner.reviseAllocation(someAlloc, 24);
    planner.reviseAllocation(someAlloc, 7);
    planner.consistencyCheck();

    heading("SCENARIO 8: CANCEL AND REVISE AN ALLOCATION");
    planner.reportAllocations();
    int firstAlloc = planner.allocationIdAt(0);
    planner.cancelAllocation(firstAlloc);
    planner.reportShelterUtilization();
    int movedAlloc = planner.allocationIdFor(9, 22);
    planner.reviseAllocation(movedAlloc, 21);
    planner.reportAllocations();
    planner.reportUnserved();
    planner.reportShelterUtilization();
    planner.consistencyCheck();
    subheading("A later change triggers a re-plan: the revision and the held people are kept");
    planner.setShelterCapacity(23, 450, true);
    planner.reportAllocations();
    planner.reportUnserved();
    planner.consistencyCheck();
    subheading("Operator releases the held people");
    planner.releaseHeld(4);
    planner.releaseHeld(4);
    planner.reportUnserved();
    planner.consistencyCheck();

    heading("SCENARIO 9: UNDO THE LATEST UPDATES (STACK, LIFO)");
    planner.printUndoStack();
    planner.undoLast();
    planner.undoLast();
    planner.printUndoStack();
    planner.reportShelterUtilization();
    planner.consistencyCheck();

    heading("SCENARIO 10: AVL INSERTION AND DELETION WITH ROTATIONS");
    planner.setShowRotations(true);
    planner.showIncidentIndex();
    subheading("Insert incidents 14..18 (ascending ids force rotations)");
    for (int id = 14; id <= 18; id++) {
        planner.registerIncident(id, 3 + (id % 5), INC_MEDICAL, 1 + (id % 3), id, "Minor_injury", -1, true);
    }
    planner.showIncidentIndex();
    subheading("Cancel two incidents and purge closed records (deletion)");
    planner.cancelIncident(15);
    planner.cancelIncident(16);
    planner.purgeClosedIncidents();
    planner.showIncidentIndex();
    planner.searchIncident(14);
    planner.searchIncident(1);
    planner.setShowRotations(false);
    planner.consistencyCheck();

    heading("FINAL REPORTS");
    allReports(planner);
    subheading("Event log (linked list)");
    planner.printEventLog();
    planner.consistencyCheck();
}

unsigned int benchSeed = 2026;

int nextRandom(int limit) {
    benchSeed = benchSeed * 1103515245 + 12345;
    return (int)((benchSeed / 65536) % 32768 * 32768 + (benchSeed / 7) % 32768) % limit;
}

double secondsSince(chrono::high_resolution_clock::time_point start) {
    chrono::duration<double> span = chrono::high_resolution_clock::now() - start;
    return span.count();
}

CampusGraph* randomGraph(int n, int extraPerNode) {
    CampusGraph* g = new CampusGraph(n);
    for (int i = 1; i < n; i++) {
        int j = nextRandom(i);
        g->addRoad(i, j, 50 + nextRandom(250), 20 + nextRandom(80), ROAD_OPEN);
    }
    int extra = n * extraPerNode;
    for (int k = 0; k < extra; k++) {
        int a = nextRandom(n);
        int b = nextRandom(n);
        int roll = nextRandom(10);
        int status = ROAD_OPEN;
        if (roll == 0) {
            status = ROAD_BLOCKED;
        } else if (roll == 1) {
            status = ROAD_CONGESTED;
        } else if (roll == 2) {
            status = ROAD_HAZARDOUS;
        }
        g->addRoad(a, b, 50 + nextRandom(250), 20 + nextRandom(80), status);
    }
    return g;
}

void benchGraph(int n) {
    CampusGraph* g = randomGraph(n, 2);
    int* order = new int[n];
    int* hops = new int[n];
    int* dist = new int[n];
    int* parent = new int[n];
    int repeats = 20000000 / (n * n) + 1;
    chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();
    for (int r = 0; r < repeats; r++) {
        g->bfs(r % n, order, hops, true);
    }
    double bfsTime = secondsSince(start) / repeats;
    start = chrono::high_resolution_clock::now();
    for (int r = 0; r < repeats; r++) {
        g->dfsOrder(r % n, order, true);
    }
    double dfsTime = secondsSince(start) / repeats;
    start = chrono::high_resolution_clock::now();
    for (int r = 0; r < repeats; r++) {
        g->dijkstra(r % n, dist, parent, true, 0);
    }
    double dijTime = secondsSince(start) / repeats;
    int pathLen = g->buildPath(parent, (repeats - 1) % n, n - 1, order);
    cout << "  " << setw(6) << n << setw(8) << g->roadCount() << setw(14) << fixed << setprecision(2) << bfsTime * 1e6
         << setw(14) << dfsTime * 1e6 << setw(16) << dijTime * 1e6 << setw(10) << pathLen
         << setw(14) << (double)n * n * 16 / 1024 << endl;
    delete[] order;
    delete[] hops;
    delete[] dist;
    delete[] parent;
    delete g;
}

void benchHeap(int n) {
    Incident* pool = new Incident[n];
    for (int i = 0; i < n; i++) {
        pool[i].id = i + 1;
        pool[i].severity = 1 + nextRandom(5);
        pool[i].priority = pool[i].severity;
        pool[i].population = nextRandom(1000);
        pool[i].timeReceived = nextRandom(100000);
        pool[i].used = true;
    }
    IncidentHeap heap(pool, n);
    chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();
    for (int i = 0; i < n; i++) {
        heap.insert(i);
    }
    double insertTime = secondsSince(start);
    bool valid = heap.isValidHeap();
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < n / 10; i++) {
        pool[i].severity = 1 + nextRandom(5);
        pool[i].priority = pool[i].severity;
        heap.updatePriority(i);
    }
    double updateTime = secondsSince(start);
    valid = valid && heap.isValidHeap();
    start = chrono::high_resolution_clock::now();
    int previous = heap.extractMax();
    bool ordered = true;
    for (int i = 1; i < n; i++) {
        int current = heap.extractMax();
        if (higherPriority(pool[current], pool[previous])) {
            ordered = false;
        }
        previous = current;
    }
    double extractTime = secondsSince(start);
    int* all = new int[n];
    for (int i = 0; i < n; i++) {
        all[i] = i;
    }
    start = chrono::high_resolution_clock::now();
    heap.buildFrom(all, n);
    double buildTime = secondsSince(start);
    valid = valid && heap.isValidHeap();
    cout << "  " << setw(8) << n << setw(14) << fixed << setprecision(3) << insertTime * 1e9 / n << setw(14)
         << extractTime * 1e9 / n << setw(14) << updateTime * 1e9 / (n / 10) << setw(16) << buildTime * 1e3
         << setw(9) << (valid && ordered ? "yes" : "NO") << endl;
    delete[] all;
    delete[] pool;
}

void benchAVL(int n) {
    int* keys = new int[n];
    for (int i = 0; i < n; i++) {
        keys[i] = i + 1;
    }
    for (int i = n - 1; i > 0; i--) {
        int j = nextRandom(i + 1);
        int temp = keys[i];
        keys[i] = keys[j];
        keys[j] = temp;
    }
    AVLTree tree("bench");
    chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();
    for (int i = 0; i < n; i++) {
        tree.insert(keys[i], i);
    }
    double insertTime = secondsSince(start);
    int height = tree.treeHeight();
    bool valid = tree.isValidAVL();
    start = chrono::high_resolution_clock::now();
    int found = 0;
    for (int i = 0; i < n; i++) {
        if (tree.search(keys[i]) != -1) {
            found++;
        }
    }
    double searchTime = secondsSince(start);
    int linearProbes = n < 20000 ? n : 20000;
    start = chrono::high_resolution_clock::now();
    int linearFound = 0;
    for (int i = 0; i < linearProbes; i++) {
        int target = keys[(int)((long long)i * n / linearProbes)];
        for (int j = 0; j < n; j++) {
            if (keys[j] == target) {
                linearFound++;
                break;
            }
        }
    }
    double linearTime = secondsSince(start);
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < n; i++) {
        tree.remove(keys[i]);
    }
    double removeTime = secondsSince(start);
    cout << "  " << setw(8) << n << setw(8) << height << setw(10) << fixed << setprecision(1)
         << 1.44 * log2((double)n + 2) << setw(12) << setprecision(3) << insertTime * 1e9 / n << setw(12)
         << searchTime * 1e9 / n << setw(12) << removeTime * 1e9 / n << setw(14) << linearTime * 1e9 / linearProbes
         << setw(8) << (valid && found == n && linearFound == linearProbes && tree.size() == 0 ? "yes" : "NO") << endl;
    delete[] keys;
}

void benchSort(int n, bool alreadySorted) {
    SortItem* original = new SortItem[n];
    SortItem* work = new SortItem[n];
    for (int i = 0; i < n; i++) {
        original[i].index = i;
        original[i].key1 = alreadySorted ? n - i : nextRandom(100000);
        original[i].key2 = nextRandom(1000);
    }
    string names[3] = {"insertion", "merge", "quick"};
    for (int algo = 0; algo < 3; algo++) {
        for (int i = 0; i < n; i++) {
            work[i] = original[i];
        }
        sortComparisons = 0;
        chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();
        if (algo == 0) {
            insertionSort(work, n);
        } else if (algo == 1) {
            mergeSort(work, n);
        } else {
            quickSort(work, n);
        }
        double t = secondsSince(start);
        long long comparisons = sortComparisons;
        bool ok = isSorted(work, n);
        cout << "  " << setw(8) << n << setw(10) << (alreadySorted ? "sorted" : "random") << setw(11) << names[algo]
             << setw(14) << fixed << setprecision(3) << t * 1e3 << setw(16) << comparisons << setw(8)
             << (ok ? "yes" : "NO") << endl;
    }
    delete[] original;
    delete[] work;
}

void runBenchmark() {
    heading("PERFORMANCE BENCHMARK (times are averages; sizes chosen to show growth)");
    subheading("Graph algorithms on adjacency matrix, about 3 roads per location");
    cout << "  " << setw(6) << "V" << setw(8) << "E" << setw(14) << "BFS (us)" << setw(14) << "DFS (us)" << setw(16)
         << "Dijkstra (us)" << setw(10) << "PathLen" << setw(14) << "Matrix (KB)" << endl;
    int graphSizes[4] = {27, 200, 800, 1600};
    for (int i = 0; i < 4; i++) {
        benchGraph(graphSizes[i]);
    }
    subheading("Binary heap priority queue");
    cout << "  " << setw(8) << "n" << setw(14) << "insert(ns)" << setw(14) << "extract(ns)" << setw(14) << "update(ns)"
         << setw(16) << "heapify(ms)" << setw(9) << "valid" << endl;
    int heapSizes[3] = {1000, 10000, 100000};
    for (int i = 0; i < 3; i++) {
        benchHeap(heapSizes[i]);
    }
    subheading("AVL tree index vs linear search");
    cout << "  " << setw(8) << "n" << setw(8) << "height" << setw(10) << "1.44lgn" << setw(12) << "insert(ns)"
         << setw(12) << "search(ns)" << setw(12) << "delete(ns)" << setw(14) << "linear(ns)" << setw(8) << "valid"
         << endl;
    int avlSizes[3] = {1000, 10000, 100000};
    for (int i = 0; i < 3; i++) {
        benchAVL(avlSizes[i]);
    }
    subheading("Sorting algorithms");
    cout << "  " << setw(8) << "n" << setw(10) << "input" << setw(11) << "algorithm" << setw(14) << "time (ms)"
         << setw(16) << "comparisons" << setw(8) << "sorted" << endl;
    int sortSizes[3] = {1000, 5000, 20000};
    for (int i = 0; i < 3; i++) {
        benchSort(sortSizes[i], false);
    }
    benchSort(5000, true);
}

void interactive(string dataFile) {
    EvacuationPlanner planner;
    if (!planner.load(dataFile)) {
        return;
    }
    heading("CAMPUS EMERGENCY EVACUATION & RESCUE RESOURCE PLANNER");
    planner.printSummary();
    while (true) {
        cout << endl;
        cout << " 1 Locations and summary        2 Roads                     3 Search location (AVL)" << endl;
        cout << " 4 Show AVL indexes             5 BFS/DFS from location     6 Connectivity / zones" << endl;
        cout << " 7 Route to nearest shelter     8 Route between two places  9 Compute evacuation plan" << endl;
        cout << "10 Change road status          11 Change shelter capacity  12 Shelter safe/unsafe" << endl;
        cout << "13 Building affected/cleared   14 Undo last update         15 Register incident" << endl;
        cout << "16 Revise incident priority    17 Cancel incident          18 Dispatch next task" << endl;
        cout << "19 Dispatch all possible       20 Complete task            21 Show priority heap" << endl;
        cout << "22 Cancel allocation           23 Revise allocation        24 Purge closed incidents" << endl;
        cout << "25 All reports                 26 Event log                27 Consistency check" << endl;
        cout << "28 Show undo stack             29 Set evacuation window    30 Release held people" << endl;
        cout << " 0 Exit" << endl;
        int choice = readInt("Choice: ", 0, 30);
        if (choice == 0) {
            break;
        } else if (choice == 1) {
            planner.printSummary();
            planner.printLocations();
        } else if (choice == 2) {
            planner.printRoads();
        } else if (choice == 3) {
            planner.searchLocation(readInt("Location id: ", -INF, INF));
        } else if (choice == 4) {
            planner.showLocationIndex();
            planner.showIncidentIndex();
        } else if (choice == 5) {
            planner.traversalReport(readInt("Start location id: ", -INF, INF));
        } else if (choice == 6) {
            planner.connectivityReport();
        } else if (choice == 7) {
            planner.routeToShelter(readInt("From location id: ", -INF, INF));
        } else if (choice == 8) {
            int a = readInt("From location id: ", -INF, INF);
            int b = readInt("To location id: ", -INF, INF);
            planner.routeBetween(a, b);
        } else if (choice == 9) {
            planner.planEvacuation(true);
            planner.reportAllocations();
            planner.reportUnserved();
        } else if (choice == 10) {
            int a = readInt("Road end 1 (location id): ", -INF, INF);
            int b = readInt("Road end 2 (location id): ", -INF, INF);
            int s = readInt("New status (1 OPEN, 2 CONGESTED, 3 HAZARDOUS, 4 BLOCKED): ", 1, 4);
            planner.setRoadStatus(a, b, s, true);
        } else if (choice == 11) {
            int id = readInt("Shelter id: ", -INF, INF);
            int cap = readInt("New capacity: ", -INF, INF);
            planner.setShelterCapacity(id, cap, true);
        } else if (choice == 12) {
            int id = readInt("Shelter id: ", -INF, INF);
            int safe = readInt("1 = safe, 0 = unsafe: ", 0, 1);
            planner.setShelterSafety(id, safe == 1, true);
        } else if (choice == 13) {
            int id = readInt("Building id: ", -INF, INF);
            int flag = readInt("1 = affected, 0 = cleared: ", 0, 1);
            planner.setAffected(id, flag == 1);
        } else if (choice == 14) {
            planner.undoLast();
        } else if (choice == 15) {
            int id = readInt("New incident id: ", -INF, INF);
            int loc = readInt("Location id: ", -INF, INF);
            int type = readInt("Type (0 FIRE, 1 GAS_LEAK, 2 STRUCTURAL, 3 MEDICAL): ", 0, 3);
            int sev = readInt("Severity (1-5): ", -INF, INF);
            int pop = readInt("Affected population: ", -INF, INF);
            string desc = readWord("Short description (one word, use _ for spaces): ");
            planner.registerIncident(id, loc, type, sev, pop, desc, -1, true);
        } else if (choice == 16) {
            int id = readInt("Incident id: ", -INF, INF);
            int sev = readInt("New severity (1-5): ", -INF, INF);
            int pop = readInt("New affected population: ", -INF, INF);
            planner.reviseIncident(id, sev, pop);
        } else if (choice == 17) {
            planner.cancelIncident(readInt("Incident id: ", -INF, INF));
        } else if (choice == 18) {
            planner.dispatchNext();
        } else if (choice == 19) {
            planner.dispatchAll();
        } else if (choice == 20) {
            planner.completeTask(readInt("Incident id: ", -INF, INF));
        } else if (choice == 21) {
            planner.showHeap();
        } else if (choice == 22) {
            planner.cancelAllocation(readInt("Allocation id: ", -INF, INF));
        } else if (choice == 23) {
            int id = readInt("Allocation id: ", -INF, INF);
            int s = readInt("New shelter id: ", -INF, INF);
            planner.reviseAllocation(id, s);
        } else if (choice == 24) {
            planner.purgeClosedIncidents();
        } else if (choice == 25) {
            allReports(planner);
        } else if (choice == 26) {
            planner.printEventLog();
        } else if (choice == 27) {
            planner.consistencyCheck();
        } else if (choice == 28) {
            planner.printUndoStack();
        } else if (choice == 29) {
            planner.setWindow(readInt("Evacuation window in minutes (1-240): ", -INF, INF), true);
        } else if (choice == 30) {
            planner.releaseHeld(readInt("Building id: ", -INF, INF));
        }
    }
    cout << "Goodbye." << endl;
}

int main(int argc, char* argv[]) {
    string mode = "menu";
    string dataFile = "campus_data.txt";
    if (argc >= 2) {
        mode = argv[1];
    }
    if (argc >= 3) {
        dataFile = argv[2];
    }
    if (mode == "demo") {
        runDemo(dataFile);
    } else if (mode == "bench") {
        runBenchmark();
    } else if (mode == "menu") {
        interactive(dataFile);
    } else {
        cout << "Usage: " << argv[0] << " [menu|demo|bench] [data file]" << endl;
        return 1;
    }
    return 0;
}
