# Campus Emergency Evacuation & Rescue Resource Planner — Technical Report

CS221 Data Structures & Algorithms, Term Project (Complex Engineering Problem)

Team: Muhammad Musab Sial (20255888), _member 2_, _member 3_

---

## 1. Problem definition

During an emergency (fire, gas leak, structural damage, severe weather), the people in the affected buildings have to be moved to safe shelters. At the same time, roads can become blocked or hazardous, shelters have limited space, and only a few rescue units are available. The planner is a decision-support simulator. It:

1. models the campus as a weighted, undirected graph whose road status can change;
2. finds which shelters each building can reach and which zones are cut off;
3. computes the cheapest safe route from each affected building and allocates evacuees to shelters without exceeding **shelter capacity or road capacity**, explaining every choice it makes when the nearest shelter or road cannot be used;
4. keeps incidents in a binary-heap priority queue and dispatches the nearest suitable free rescue unit to the most urgent one;
5. accepts live changes (blockages, capacity changes, new incidents, cancellations, revisions) and re-plans without corrupting records;
6. produces sorted reports.

The objectives conflict (CEP attribute P2). The shortest route may be hazardous. The nearest shelter may be full. The most severe incident may affect few people. A free unit may be far away. Section 4 explains which rule the planner uses to settle each conflict and why.

## 2. Assumptions

| # | Assumption | Reason |
|---|---|---|
| A1 | Roads are two-way, so the graph is undirected and the matrix is symmetric. | Campus roads and corridors are walked in both directions. |
| A2 | Travel cost = distance × status factor: OPEN ×1, CONGESTED ×2, HAZARDOUS ×4, BLOCKED = ∞. | A single number lets Dijkstra trade distance against danger. The factors are tunable constants. |
| A3 | Evacuees try safe roads first. A hazardous road is used only when no safe route to a free shelter exists, and the route is then flagged with a WARNING. | Safety ranks above speed, but leaving people inside is worse than a risky route. |
| A4 | Only SHELTER nodes receive evacuees. Gates and junctions are pass-through nodes. | Shelters are the only places with a capacity that is tracked. |
| A5 | Walking speed is 80 cost units/min. Road capacity is in people/min. Estimated time = cost/80 + people/bottleneck. | Gives a rough evacuation time that accounts for queueing at the narrowest road. |
| A5b | The plan has an **evacuation window** W (default 10 min, changeable at run time). A road may carry at most capacity × W people in total. | Capacity is a flow rate (people/min), so it becomes a hard limit only over a time window. "Everyone through every road within W minutes" is the command-level target, and tightening W shows its effect directly. |
| A6 | Each incident type needs one kind of unit: FIRE → fire truck, MEDICAL → ambulance, GAS_LEAK/STRUCTURAL → rescue team. | Realistic matching. It also makes "insufficient resources" a real case. |
| A7 | Rescue vehicles may drive on hazardous roads, at ×4 cost. | Responders are equipped for danger; evacuees are not. |
| A8 | When a task is completed, the unit stays at the incident location and starts its next job from there. | Keeps the unit's current location realistic. |
| A9 | Any network or shelter change triggers a full re-plan. Operator decisions survive it: a revised allocation is pinned and re-applied first if it is still feasible (otherwise it is dropped with a message), and people whose allocation was cancelled stay held until released. | Rebuilding from scratch guarantees consistency (no stale loads or occupancies). Re-applying pins keeps the operator in control. A re-plan takes microseconds on this campus. |
| A10 | Time is a simulation clock that ticks one minute per operation. | Gives a deterministic "time received" for the tie-break and for aging. |
| A11 | Aging: every 30 minutes of waiting raises a pending task's priority by 1, at most +2, and never above 4 unless its true severity is 5. Ties on priority are broken by true severity. | Prevents starvation without letting an aged task overtake a genuinely more severe one or a real severity-5 emergency. |

## 3. System architecture

```
                       +--------------------------+
   campus_data.txt --> |   EvacuationPlanner      | <-- menu / demo (main.cpp)
                       |  (controller, all rules) |
                       +------------+-------------+
          +-------------+-----------+-----------+--------------+-------------+
          v             v           v           v              v             v
   CampusGraph     AVLTree x2   IncidentHeap  Sorting     QueueStack     Linked lists
   adjacency       location id  binary max-   insertion   IntQueue (BFS) Allocation list
   matrix Edge**   -> index;    heap of slot  merge       IntStack (path) Event log
   BFS, DFS,       incident id  indices +     quick       UndoStack
   Dijkstra        -> slot      position[]
          |
          v
   Location[] array   Incident[] slot array   Resource[] array
```

### Source files

Every class has a header with declarations only (`.h`) and a separate implementation file (`.cpp`). The planner is split by responsibility:

| File | Responsibility |
|---|---|
| `Structures.h/.cpp` | Records, constants, name/parse helpers |
| `QueueStack.h/.cpp` | Linked queue, linked stack, undo stack |
| `AVLTree.h/.cpp` | AVL tree |
| `IncidentHeap.h/.cpp` | Binary max-heap with `position[]`, priority comparator |
| `Sorting.h/.cpp` | Insertion, merge, quick sort |
| `CampusGraph.h/.cpp` | Adjacency matrix, BFS, DFS, components, Dijkstra, path |
| `EvacuationPlanner.h` | Planner class declaration |
| `EvacuationPlanner.cpp` | Loading and validating data, indexes, summary, event log |
| `RouteAnalysis.cpp` | Traversals, connectivity, route queries |
| `ShelterAllocation.cpp` | Evacuation planning, allocation records, pins, held people |
| `DynamicUpdates.cpp` | Road, shelter, window and affected-building updates; undo |
| `IncidentDispatch.cpp` | Incidents, aging, priority queue, dispatch, completion, purge |
| `Reports.cpp` | All reports and the consistency check |
| `main.cpp` | Menu, demo scenarios, benchmark |

### Data structure diagram

```
Location array (index 0..V-1)          Adjacency matrix V x V (Edge)
+----+-------------+---------+         +-----+-----+-----+
| 0  | Main_Gate   | GATE    |   <-->  |     | 0   | 1  ...   adj[u][v] = {distance, capacity,
| 1  | Back_Gate   | GATE    |         | 0   | -   | ...           status, load}
| .. | ...         |         |         | 1   |     |               adj[v][u] mirrors it
+----+-------------+---------+         +-----+-----+

Location AVL (key = location id, value = array index)     Incident AVL (key = incident id, value = slot)
             8                                                     8
          /     \                                               /     \
        4         16 ...                                       4       12 ...

Incident slot array  [ slot0 | slot1 | ... | slot199 ]   used flag allows slot reuse
IncidentHeap.heap    [ 11 | 4 | 0 | 7 | ... ]           slot numbers, max-heap order
IncidentHeap.position[slot] = index in heap, or -1       makes update/remove O(log n)

Allocation linked list:  head -> [id 1 | Hostel_B -> Open_Lawn | 250 | path*] -> [id 2 ...] -> NULL
Undo stack (linked):     top  -> [Road X-Y: OPEN->BLOCKED] -> [Shelter Z cap 900->500] -> NULL
```

### Data-structure justification

| Need | Structure chosen | Why this one |
|---|---|---|
| Campus network | Adjacency matrix `Edge**` | Required by the brief. O(1) to read or change the status of any road, which is the operation done most during an emergency. The campus graph is small (27 nodes = 11 KB). |
| Locations, resources | Arrays | Fixed size after loading, O(1) access by index, and they double as matrix row numbers. |
| ID → record lookup | AVL trees | IDs come from the data file and are not 0..n−1, so an index is needed. An AVL tree guarantees O(log n) height, whereas a plain BST degenerates to a list when IDs arrive in sorted order, which is exactly what happens with sequential incident IDs (see the rotations in Scenario 10). |
| Incidents | Slot array + AVL | Incidents are added and deleted at run time. Slots are reused and the AVL maps id → slot, so delete is O(log n) and nothing has to be shifted. |
| Pending tasks | Binary max-heap | Dispatch always needs the most urgent task: O(log n) insert and extract. A `position[]` array makes priority update and cancellation O(log n) as well, instead of O(n). |
| Allocations, event log | Singly linked lists | Records are inserted and deleted in the middle (cancel), and the count is unknown in advance. |
| BFS frontier | Linked queue | FIFO order gives level-by-level (hop-count) exploration. |
| Route reconstruction | Linked stack | `parent[]` gives the path backwards. Pushing it onto a stack and popping reverses it (LIFO). |
| Undo | Linked stack of records | The latest change must be undone first (LIFO). |
| Reports | Three hand-written sorts | Insertion sort for small or nearly sorted lists (candidate shelters, bottlenecks). Merge sort when stability and a guaranteed n log n matter (evacuation order, utilization). Quick sort for incident and risk rankings. |

## 4. Algorithms and decision rules

### 4.1 Route analysis

* **BFS** (queue) from a building over safe roads lists the reachable shelters together with their hop distance.
* **Recursive DFS** labels connected components with blocked roads removed. A component with no safe shelter is reported as a **DISCONNECTED zone** (the Observatory in the dataset).
* **Dijkstra**, array version: each of the V rounds picks the closest unfinished node by a linear scan, then relaxes its matrix row. With an adjacency matrix every row scan is already O(V), so a heap-based Dijkstra would not improve on O(V²) here. The simple version is therefore both correct and the best choice for this representation.
* **Path reconstruction**: walk `parent[]` from the shelter back to the source, pushing each node onto a stack, then pop to get the forward path.

### 4.2 Evacuation and shelter allocation

```
planEvacuation():
    remember pinned (operator-revised) allocations, then clear everything
    re-apply each pin if the shelter is safe, has space and a route has spare road capacity
    order affected buildings by (incident danger score DESC, population DESC)     -- merge sort
    for each building b (people waiting = population - evacuated - held):
        for pass in [safe roads only, hazardous allowed]:
            repeat while people are waiting:
                Dijkstra from b, ignoring roads whose load has reached capacity x W
                candidate shelters sorted by cost ASC                              -- insertion sort
                first candidate that is safe and not full = s (others recorded as UNSAFE / FULL)
                if none: next pass
                take = min(waiting, free space of s, smallest spare road capacity on the route)
                store the route, add the load to every road on it, write the reason
        if people remain: reason = disconnected / all reachable shelters full or unsafe /
                          shelters have space but every road to them is at capacity
```

Each repetition either empties the building, fills a shelter or saturates a road. So the loop runs at most S + E times per pass for each building.

**Trade-off decisions.**
1. Buildings with the most dangerous incidents are planned first, so they get the closest shelters. Distance is not used for ordering, because a nearby building with no incident can wait a few minutes longer.
2. Evacuees are split across shelters instead of being refused. This keeps the number of unserved people as small as possible.
3. A hazardous route is used only as a last resort, and it is flagged.

4. Road capacity is a hard limit. When a road fills up, Dijkstra runs again without it, so the rest of the people take the next-best route instead of queueing for ever on the shortest one.

Neither kind of capacity can be exceeded, because every allocation takes at most `capacity − occupancy` shelter places and at most `capacity × W − load` on every road of its route. `consistencyCheck()` verifies both.

### 4.3 Emergency prioritization: the priority rule

An incident X comes before Y if:

1. its **priority** is higher — priority = true severity (1–5) plus aging (below);
2. otherwise, its **true severity** is higher — an aged task never overtakes a genuinely more severe one;
3. otherwise, **affected population** is higher — more lives at stake;
4. otherwise, it was **received earlier** — fairness (FIFO among equals);
5. otherwise, it has the smaller ID — a deterministic final tie-break.

A lexicographic key was chosen over a weighted sum (for example `severity×100 + population`) because a weighted sum lets a large but minor incident outrank a small life-threatening one. With the lexicographic key, severity can never be outweighed, and the rule is easy to defend to stakeholders.

**Aging against starvation.** Without aging, a severity-1 task could wait for ever behind a stream of more urgent ones. Every 30 minutes of waiting raises a pending task's priority by one, at most +2, and never to 5 unless its true severity is 5. Before each dispatch, `applyAging()` updates these priorities and rebuilds the heap bottom-up in O(n), which is the brief's "update/rebuild priority" operation. In the demo, incident 9 (severity 2) rises to priority 3 after waiting 41 minutes.

### 4.4 Resource assignment

```
dispatchNext():
    repeat: extract the top incident from the heap
        Dijkstra from the incident location (hazardous allowed for vehicles)
        among free units of the required type, pick the one with the lowest travel cost
        if one is found: mark the unit BUSY, the incident DISPATCHED, print the route; stop
        else: record why (all busy / cut off / none of this type) and keep it aside
    put the skipped incidents back into the heap
```

Skipping incidents that cannot be served avoids head-of-line blocking. For example, if no fire truck is free, an ambulance can still be sent to the next medical case. Double assignment is impossible because:
- a unit is picked only when `busy == false`;
- a dispatched incident is no longer in the heap;
- `consistencyCheck()` verifies that every busy unit owns exactly one dispatched incident.

### 4.5 Dynamic updates and consistency

Supported operations:
- road status change (add or remove a blockage);
- shelter capacity change and shelter safety change;
- change of the evacuation window W;
- mark a building affected or cleared;
- register, revise and cancel an incident;
- dispatch and complete a task;
- cancel an allocation (its people are held in place) and release held people;
- revise an allocation to another shelter (pinned, kept across re-plans while feasible);
- purge closed incidents (AVL deletion);
- undo the latest road, shelter or window update.

Undo covers the network and shelter updates because those change the environment, and a wrong entry there (the wrong road blocked) is the mistake an operator most needs to reverse quickly. Each incident and allocation operation already has its own inverse (cancel, release, revise), which the brief's "latest supported update" allows.

Every input is validated before anything is changed. The following are all rejected with a message: invalid IDs, a severity outside 1–5, negative or out-of-range values (people ≤ 100,000, road capacity ≤ 10,000/min, distance ≤ 100,000, window 1–240 min, which also rules out integer overflow), duplicate IDs, a road that does not exist, a non-shelter target, an unsafe or full target shelter, a route without spare road capacity, and completing a task that was never dispatched.

`consistencyCheck()` recomputes shelter occupancy, building evacuated counts and every road's load from the allocation list, and compares them with the stored values. It also checks that no shelter or road is over capacity, that evacuated + held never exceeds a building's population, that no unit is double-assigned, and that the heap and both AVL trees are still valid. It runs 14 times in the demo and passes every time.

## 5. Complexity analysis

V = locations, E = roads, n = items in a structure, B = affected buildings, S = shelters, R = resources.

| Operation | Time | Space | Notes |
|---|---|---|---|
| Change road status / read road | O(1) | – | direct matrix cell |
| BFS | O(V²) | O(V) queue | every dequeued node scans its matrix row |
| DFS (recursive) / components | O(V²) | O(V) recursion stack | |
| Dijkstra (array) | O(V²) | O(V) | optimal for a matrix |
| Path reconstruction | O(V) | O(V) stack | |
| Plan evacuation | O(B·(S+E)·(V² + S²)) worst case | O(V + allocations) | each Dijkstra round fills a shelter, saturates a road or finishes the building; usually only a few rounds per building |
| Aging + rebuild | O(n) | – | bottom-up heapify |
| Consistency check | O(V² + A·V + R·n) | O(V²) | A = allocations |
| Heap insert / extract / update / remove | O(log n) | O(n) | position[] gives O(log n) update and remove |
| Heapify (rebuild) | O(n) | – | bottom-up |
| AVL insert / delete / search | O(log n) | O(n) | height ≤ 1.44 log₂ n |
| AVL traversals | O(n) | O(log n) recursion | |
| Dispatch one task | O(k·(V² + log n) + R) | O(V) | k = tasks skipped first |
| Insertion sort | O(n²) average/worst, O(n) best | O(1) | |
| Merge sort | O(n log n) all cases | O(n) | stable |
| Quick sort (middle pivot) | O(n log n) average, O(n²) worst | O(log n) stack | |
| Adjacency matrix storage | – | O(V²) | 16 bytes per cell |

## 6. Measured performance

`./planner bench` on an Intel Xeon @ 2.10 GHz, `g++ -O2`. Random connected graphs with about 3 roads per node: 10% blocked, 10% congested, 10% hazardous.

**Graph algorithms** (µs per run):

| V | E | BFS | DFS | Dijkstra | Matrix |
|---|---|---|---|---|---|
| 27 (real campus size) | 69 | 1.4 | 0.9 | 5.8 | 11 KB |
| 200 | 582 | 59 | 66 | 203 | 625 KB |
| 800 | 2391 | 700 | 777 | 2029 | 10 MB |
| 1600 | 4790 | 3363 | 3273 | 8345 | 40 MB |

Doubling V from 800 to 1600 multiplies Dijkstra's time by 4.1 and BFS's by 4.8, which matches O(V²). The matrix memory also grows ×4, and this is the real limit on scale (Section 9).

**Binary heap** (ns per operation):

| n | insert | extract | update | heapify (ms) |
|---|---|---|---|---|
| 1,000 | 30 | 150 | 71 | 0.025 |
| 10,000 | 31 | 232 | 58 | 0.269 |
| 100,000 | 44 | 463 | 106 | 4.11 |

Extract grows slowly, as O(log n) predicts. Insert stays almost flat because a random key sifts up only about 1–2 levels on average; O(log n) is its worst case. Heapify grows linearly (×10 per ×10 n), which confirms O(n).

**AVL tree vs linear search** (ns per operation):

| n | height | 1.44 log₂ n | insert | search | delete | linear search |
|---|---|---|---|---|---|---|
| 1,000 | 12 | 14.4 | 121 | 25 | 112 | 218 |
| 10,000 | 16 | 19.1 | 176 | 38 | 182 | 1,935 |
| 100,000 | 20 | 23.9 | 437 | 69 | 357 | 18,559 |

The tree height stays below the AVL bound. Linear search grows ×10 per ×10 n, while AVL search less than doubles. At n = 100,000 the AVL is about 270 times faster.

**Sorting** (ms; comparisons):

| n | input | insertion | merge | quick |
|---|---|---|---|---|
| 1,000 | random | 0.29 (250,254) | 0.08 (8,701) | 0.06 (10,389) |
| 5,000 | random | 6.84 (6.3 M) | 0.50 (55,223) | 0.44 (71,735) |
| 20,000 | random | 143.8 (99.4 M) | 3.06 (260,839) | 2.30 (327,277) |
| 5,000 | already sorted | **0.017** (4,999) | 0.21 (32,004) | 0.13 (51,822) |

Insertion sort grows about ×16–21 per ×4 n (quadratic), but it is the fastest on sorted input (O(n) best case). Quick sort makes more comparisons than merge sort yet runs faster, because it sorts in place and so has lower constants and better cache use. Merge sort is used where stability matters.

**Comparison summary.** Insertion sort is right for the tiny lists inside the planner (at most 5 candidate shelters, at most 38 roads). Merge or quick sort is needed for anything at the scale of thousands.

## 7. Test cases and results

Run `./planner demo`. The consistency check runs 14 times across the scenarios and passes every time. `valgrind --leak-check=full` reports 0 leaks and 0 errors. All route costs were cross-checked against an independent Python Dijkstra.

The dataset has 27 locations, 38 roads, 5 shelters (total capacity 2,550), 6 resources and 12 incidents. 2,445 people are in the 9 affected buildings.

| # | Required test | How it is triggered | Result observed |
|---|---|---|---|
| 1 | Normal scenario | Load, AVL index, BFS/DFS, connectivity, a route for every affected building | 2 zones found. Every affected building except the Observatory gets a route, e.g. Hostel_A → Football_Ground, cost 120. |
| 2 | Blocked shortest route | Block Hostel_A–Football_Ground, then make Junction_East–Football_Ground hazardous | The route is recomputed: cost 120 → 300 via Junction_East, then → Open_Lawn (310), because the ×4 hazard penalty raises the old route to 160 + 4×140 = 720. |
| 3 | Full / unsafe nearest shelter | Plan: 2,420 of 2,445 placed. Mark Assembly_North UNSAFE, cut Cricket_Ground to 500 | Every allocation explains itself, e.g. *"skipped Assembly_North (cost 210) is UNSAFE; Cricket_Ground (cost 280) is FULL; …chose Football_Ground (cost 510)"*. Unserved count rises with a reason; capacity is never exceeded. |
| 3b | Road capacity | Bottleneck report at W = 10 min, then W tightened to 6 min, then undone | At 10 min several roads are at 100% of their limit and people are routed around them (*"split, road Junction_West - Sports_Complex reaches its capacity limit"*). At 6 min: 1,840 placed, 605 unserved, with the reason *"shelters with space exist but every road to them is at capacity"*. Undo restores 1,850 placed. |
| 4 | Multiple incidents, different priorities | 12 incidents. Heap root = #12 (sev 5, pop 350), ahead of #1 (sev 5, pop 90) | Dispatch follows the priority rule; each unit is the nearest free one of the right type. |
| 5 | Insufficient resources | 4 fire, 4 medical and 4 rescue incidents, but only 2 units of each type | *"insufficient resources: all 2 FIRE_TRUCK units are busy"*. The incident stays pending; lower-priority incidents of other types are still served. |
| 6 | Disconnected location | Observatory's only road is BLOCKED | DFS reports the zone as DISCONNECTED; route query: *"unreachable (all roads to it are blocked)"*; 25 people unserved with the reason. |
| 7 | Change while running | Reopen the Observatory road; complete tasks; new sev-5 incident #13; revise #11 from sev 1 to 4; incidents left waiting | Plan recomputed. #13 becomes the heap root at once and #11 moves up; freed units are reassigned from their new locations. Waiting tasks age (#3: 3 → 4 after 30 min) but stay behind genuine severity-4 tasks. |
| 8 | Invalid input | Duplicate ID, location 99, severity 9, population −4, capacity −5, non-shelter, non-existent road, status 7, completing a pending task, allocation 999, full/unsafe/non-shelter revision targets; in the menu, window 0 and 500 | Each one is rejected with a specific message, and state is unchanged (consistency check passes). |
| 9 | Cancel / revise allocation | Cancel allocation 1 (Library's 200 people are held), move Lecture_Hall's 150 people into the space it freed, then change Parking_Lot's capacity to force a re-plan, then release the held people | Occupancies, road loads and unserved counts update correctly. The revision survives the re-plan (*"operator revision kept"*) and the 200 stay held until released. Releasing twice is rejected. |
| 10 | Undo (stack) | Undo the latest 2 updates | Reverted in LIFO order (road reopening first, then the capacity change), and the plan is recomputed. |
| 11 | AVL rotations and deletion | Insert incidents 14–18 (ascending), cancel 2, purge 6 closed records | RR cases and left rotations are printed; height stays at 5 for 18 keys; it is still a valid AVL after the deletions. |
| 12 | Interactive input validation | Letters or out-of-range numbers typed at the menu | Re-prompts instead of crashing. Malformed data files are rejected with the line at fault. |

## 8. Reports produced

Shown at the end of the demo, and available from menu option 25:

- evacuation assignments (route, cost, estimated minutes, reason);
- unserved people with the reason, and people held by the operator;
- shelter utilization (**merge sort**);
- resource status;
- critical bottlenecks — roads ranked by load/capacity, i.e. minutes needed to clear, with % of the road limit (**insertion sort**);
- ranked high-risk zones, with risk = 10·Σseverity² + unserved/10 + 50 if cut off (**quick sort**);
- incidents ranked by the priority rule (**quick sort**);
- event log (linked list) and the undo stack.

## 9. Limitations and possible improvements

- **O(V²) memory** for the adjacency matrix: 1,600 nodes already need 40 MB. A city-scale map would need an adjacency list, where Dijkstra with a heap runs in O((V+E) log V).
- **Greedy allocation**, building by building. It is not globally optimal: a later building may be sent far away because an earlier one took its nearest shelter. A min-cost-flow formulation would minimise the total evacuation time, but it is outside this course.
- **Road capacity is a total over the window, not a time-by-time schedule.** The planner guarantees that no road carries more than capacity × W people, but it does not schedule *when* each group walks. A time-expanded network would model this, at a much higher cost.
- **Travel cost does not rise smoothly with load.** A road is either usable or full. A load-dependent cost would spread traffic earlier, at the price of no longer having a simple shortest-path rule.
- **Undo** covers road, shelter and window updates. Incident and allocation operations are reversed with their own inverse operations rather than through the stack.
- Single-threaded, discrete simulation. Travel and task durations are not simulated in real time.

## 10. CEP attribute mapping

| Attribute | Where it appears |
|---|---|
| P1 Depth of knowledge | Graph (BFS, DFS, Dijkstra), AVL, heap, queue, stack, linked lists, 3 sorts, recursion, measured complexity. |
| P2 Conflicting requirements | Safety vs speed (hazard penalty and last-resort pass), shelter and road capacity vs nearest shelter (splitting and rerouting), severity vs population (priority rule), urgency vs fairness (aging), urgency vs distance (dispatch), automation vs operator control (pins and held people). |
| P3 Depth of analysis | The cost and priority rules are defined and justified in Section 4. The alternatives (weighted priority sum, heap-based Dijkstra, adjacency list, min-cost flow) are evaluated against the chosen rules. |
| P6 Stakeholders | Evacuees (routes, times), administration (utilization, unserved), security (bottlenecks, disconnected zones), rescue teams (dispatch routes), facility managers (shelter capacity and safety). |
| P7 Interdependence | A road change re-runs routing, then allocation, then bottlenecks and risk. An incident changes the evacuation order and the dispatch queue. Completing a task moves the unit, which changes the next dispatch. |
