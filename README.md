# Campus Emergency Evacuation & Rescue Resource Planner

CS221 Data Structures & Algorithms term project (CEP), C++ implementation.

## Build

```
g++ -std=c++11 -Wall -Wextra -O2 *.cpp -o planner
```

No external libraries and no STL containers. Every data structure is written by hand.

## Run

| Command | What it does |
|---|---|
| `./planner` or `./planner menu` | Interactive menu (30 operations) on `campus_data.txt` |
| `./planner demo` | Scripted demonstration of all required test scenarios (11 scenarios + final reports) |
| `./planner bench` | Performance benchmark on several dataset sizes |
| `./planner demo other_data.txt` | Use a different dataset file |

## Files

Each data structure has a header (`.h`, declarations) and an implementation file (`.cpp`).

| File | Contents |
|---|---|
| `Structures.h/.cpp` | Records (Location, Edge, Incident, Resource, Allocation, SortItem), constants, name/parse helpers |
| `QueueStack.h/.cpp` | Linked-list queue (BFS), linked-list stack (route reconstruction), undo stack |
| `AVLTree.h/.cpp` | AVL tree: insert, delete, search, three traversals, rotations, shape print |
| `IncidentHeap.h/.cpp` | Binary max-heap priority queue with position index (insert, extract, update, remove, heapify) |
| `Sorting.h/.cpp` | Insertion sort, merge sort, quick sort |
| `CampusGraph.h/.cpp` | Adjacency-matrix graph, BFS, recursive DFS, connected components, capacity-aware Dijkstra, path reconstruction |
| `EvacuationPlanner.h` | Planner class declaration |
| `EvacuationPlanner.cpp` | Data loading and validation, indexes, summary, event log |
| `RouteAnalysis.cpp` | Traversals, connectivity, route queries |
| `ShelterAllocation.cpp` | Evacuation planning with shelter and road capacity, allocation records, pinned revisions, held people |
| `DynamicUpdates.cpp` | Road, shelter, window and building updates, undo |
| `IncidentDispatch.cpp` | Incidents, priority aging, dispatch, completion, purge |
| `Reports.cpp` | Reports and the consistency check |
| `main.cpp` | Menu, demo scenarios, benchmark |
| `campus_data.txt` | Dataset: 27 locations, 38 roads, 5 shelters, 6 resources, 12 incidents |
| `REPORT.md` | Technical report |
