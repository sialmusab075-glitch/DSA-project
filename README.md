# Campus Emergency Evacuation & Rescue Resource Planner

CS221 Data Structures & Algorithms term project (CEP), C++ implementation.

## Build

```
g++ -std=c++11 -Wall -Wextra -O2 main.cpp -o planner
```

No external libraries and no STL containers. Every data structure is written by hand.

## Run

| Command | What it does |
|---|---|
| `./planner` or `./planner menu` | Interactive menu (28 operations) on `campus_data.txt` |
| `./planner demo` | Scripted demonstration of all required test scenarios (10 scenarios + final reports) |
| `./planner bench` | Performance benchmark on several dataset sizes |
| `./planner demo other_data.txt` | Use a different dataset file |

## Files

| File | Contents |
|---|---|
| `Structures.h` | Records (Location, Edge, Incident, Resource, Allocation, SortItem), constants, name/parse helpers |
| `QueueStack.h` | Linked-list queue (BFS), linked-list stack (route reconstruction), undo stack |
| `AVLTree.h` | AVL tree: insert, delete, search, three traversals, rotations, shape print |
| `IncidentHeap.h` | Binary max-heap priority queue with position index (insert, extract, update, remove, heapify) |
| `Sorting.h` | Insertion sort, merge sort, quick sort |
| `CampusGraph.h` | Adjacency-matrix graph, BFS, recursive DFS, connected components, Dijkstra, path reconstruction |
| `EvacuationPlanner.h` | The system: data loading, evacuation planning, shelter allocation, dispatch, updates, undo, reports |
| `main.cpp` | Menu, demo scenarios, benchmark |
| `campus_data.txt` | Dataset: 27 locations, 38 roads, 5 shelters, 6 resources, 12 incidents |
| `REPORT.md` | Technical report |
