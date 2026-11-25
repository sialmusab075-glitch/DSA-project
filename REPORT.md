# Campus Emergency Evacuation & Rescue Resource Planner — Technical Report

CS221 Data Structures & Algorithms, Term Project (Complex Engineering Problem)

Team: Muhammad Musab Sial (20255888), _member 2_, _member 3_

---

## 1. Problem definition

During an emergency (fire, gas leak, structural damage, severe weather), the people in the affected buildings have to be moved to safe shelters. At the same time, roads can become blocked or hazardous, shelters have limited space, and only a few rescue units are available. The planner is a decision-support simulator. It:

1. models the campus as a weighted, undirected graph whose road status can change;
2. finds which shelters each building can reach and which zones are cut off;
3. computes the cheapest safe route from each affected building and allocates evacuees to shelters without exceeding capacity, explaining every choice it makes when the nearest shelter cannot be used;
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
| A6 | Each incident type needs one kind of unit: FIRE → fire truck, MEDICAL → ambulance, GAS_LEAK/STRUCTURAL → rescue team. | Realistic matching. It also makes "insufficient resources" a real case. |
| A7 | Rescue vehicles may drive on hazardous roads, at ×4 cost. | Responders are equipped for danger; evacuees are not. |
| A8 | When a task is completed, the unit stays at the incident location and starts its next job from there. | Keeps the unit's current location realistic. |
| A9 | Any network or shelter change triggers a full re-plan, which discards manual allocation edits. | Consistency is guaranteed. The cost is one re-plan per change, which takes microseconds here. |
| A10 | Time is a simulation clock that ticks once per operation. | Gives a deterministic "time received" for the priority tie-break. |

## 3. System architecture

```
                       +--------------------------+
   campus_data.txt --> |   EvacuationPlanner      | <-- menu / demo (main.cpp)
                       |  (controller, all rules) |
                       +------------+-------------+
          +-------------+-----------+-----------+--------------+-------------+
          v             v           v           v              v             v
   CampusGraph     AVLTree x2   IncidentHeap  Sorting.h   QueueStack.h   Linked lists
   adjacency       location id  binary max-   insertion   IntQueue (BFS) Allocation list
   matrix Edge**   -> index;    heap of slot  merge       IntStack (path) Event log
   BFS, DFS,       incident id  indices +     quick       UndoStack
   Dijkstra        -> slot      position[]
          |
          v
   Location[] array   Incident[] slot array   Resource[] array
```

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
    clear all allocations, occupancies and road loads
    order affected buildings by (incident danger score DESC, population DESC)     -- merge sort
    for each building b:
        for pass in [safe roads only, hazardous allowed]:
            Dijkstra from b
            candidate shelters sorted by cost ASC                                  -- insertion sort
            for each candidate s:
                if s unsafe:  record "s is UNSAFE", skip
                if s full:    record "s is FULL", skip
                allocate min(remaining, free space of s), store route, add road loads
                reason = "nearest feasible" or "skipped <list>; chose s"
        if people remain: record reason ("disconnected zone" or "all reachable shelters full/unsafe")
```

**Trade-off decisions.**
1. Buildings with the most dangerous incidents are planned first, so they get the closest shelters. Distance is not used for ordering, because a nearby building with no incident can wait a few minutes longer.
2. Evacuees are split across shelters instead of being refused. This keeps the number of unserved people as small as possible.
3. A hazardous route is used only as a last resort, and it is flagged.

Capacity is never exceeded, because every allocation takes at most `capacity − occupancy` places.

### 4.3 Emergency prioritization: the priority rule

An incident X comes before Y if:

1. **severity** (1–5) is higher — the threat to life comes first;
2. otherwise, **affected population** is higher — more lives at stake;
3. otherwise, it was **received earlier** — fairness (FIFO among equals);
4. otherwise, it has the smaller ID — a deterministic final tie-break.

A lexicographic key was chosen over a weighted sum (for example `severity×100 + population`) because a weighted sum lets a large but minor incident outrank a small life-threatening one. With the lexicographic key, severity can never be outweighed, and the rule is easy to defend to stakeholders. Its known weakness is **starvation**: a severity-1 task can wait for ever. Section 9 discusses aging as the fix.

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
- mark a building affected or cleared;
- register, revise and cancel an incident;
- dispatch and complete a task;
- cancel and revise an allocation;
- purge closed incidents (AVL deletion);
- undo the latest network or shelter update.

Every input is validated before anything is changed. Invalid IDs, a severity outside 1–5, negative values, duplicate IDs, a road that does not exist, a non-shelter target, an unsafe or full target shelter, and completing a task that was never dispatched are all rejected with a message.

`consistencyCheck()` recomputes shelter occupancy and building evacuated counts from the allocation list. It also checks that no shelter is over capacity, that no unit is double-assigned, and that the heap and both AVL trees are still valid. It runs after every demo scenario and passes every time.

## 5. Complexity analysis

V = locations, E = roads, n = items in a structure, B = affected buildings, S = shelters, R = resources.

| Operation | Time | Space | Notes |
|---|---|---|---|
| Change road status / read road | O(1) | – | direct matrix cell |
| BFS | O(V²) | O(V) queue | every dequeued node scans its matrix row |
| DFS (recursive) / components | O(V²) | O(V) recursion stack | |
| Dijkstra (array) | O(V²) | O(V) | optimal for a matrix |
| Path reconstruction | O(V) | O(V) stack | |
| Plan evacuation | O(B·V² + B·S²) | O(V + allocations) | 2 Dijkstra runs + insertion sort of shelters per building |
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
| 27 (real campus size) | 69 | 1.2 | 0.8 | 5.1 | 11 KB |
| 200 | 582 | 67 | 64 | 182 | 625 KB |
| 800 | 2391 | 830 | 827 | 1833 | 10 MB |
| 1600 | 4790 | 3705 | 3344 | 7502 | 40 MB |

Doubling V from 800 to 1600 multiplies Dijkstra's time by 4.1 and BFS's by 4.5, which matches O(V²). The matrix memory also grows ×4, and this is the real limit on scale (Section 9).

**Binary heap** (ns per operation):

| n | insert | extract | update | heapify (ms) |
|---|---|---|---|---|
| 1,000 | 27 | 113 | 63 | 0.024 |
| 10,000 | 27 | 265 | 54 | 0.231 |
| 100,000 | 29 | 419 | 82 | 3.43 |

Extract grows slowly, as O(log n) predicts. Insert stays almost flat because a random key sifts up only about 1–2 levels on average; O(log n) is its worst case. Heapify grows linearly (×10 per ×10 n), which confirms O(n).

**AVL tree vs linear search** (ns per operation):

| n | height | 1.44 log₂ n | insert | search | delete | linear search |
|---|---|---|---|---|---|---|
| 1,000 | 12 | 14.4 | 121 | 24 | 109 | 193 |
| 10,000 | 16 | 19.1 | 163 | 39 | 161 | 1,863 |
| 100,000 | 20 | 23.9 | 295 | 67 | 327 | 18,741 |

The tree height stays below the AVL bound. Linear search grows ×10 per ×10 n, while AVL search less than doubles. At n = 100,000 the AVL is about 280 times faster.

**Sorting** (ms; comparisons):

| n | input | insertion | merge | quick |
|---|---|---|---|---|
| 1,000 | random | 0.28 (250,254) | 0.08 (8,701) | 0.06 (10,389) |
| 5,000 | random | 7.37 (6.3 M) | 0.50 (55,223) | 0.37 (71,735) |
| 20,000 | random | 108.5 (99.4 M) | 2.36 (260,839) | 1.92 (327,277) |
| 5,000 | already sorted | **0.010** (4,999) | 0.13 (32,004) | 0.09 (51,822) |

Insertion sort grows about ×15 per ×4 n (quadratic), but it is the fastest on sorted input (O(n) best case). Quick sort makes more comparisons than merge sort yet runs faster, because it sorts in place and so has lower constants and better cache use. Merge sort is used where stability matters.

**Comparison summary.** Insertion sort is right for the tiny lists inside the planner (at most 5 candidate shelters, at most 38 roads). Merge or quick sort is needed for anything at the scale of thousands.

## 7. Test cases and results

Run `./planner demo`. Every scenario ends with a consistency check, and all 10 checks pass. `valgrind --leak-check=full` reports 0 leaks and 0 errors. All route costs were cross-checked against an independent Python Dijkstra.

The dataset has 27 locations, 38 roads, 5 shelters (total capacity 2,550), 6 resources and 12 incidents. 2,445 people are in the 9 affected buildings.

| # | Required test | How it is triggered | Result observed |
|---|---|---|---|
| 1 | Normal scenario | Load, AVL index, BFS/DFS, connectivity, a route for every affected building | 2 zones found. Every affected building except the Observatory gets a route, e.g. Hostel_A → Football_Ground, cost 120. |
| 2 | Blocked shortest route | Block Hostel_A–Football_Ground, then make Junction_East–Football_Ground hazardous | The route is recomputed: cost 120 → 300 via Junction_East, then → Open_Lawn (310), because the ×4 hazard penalty raises the old route to 160 + 4×140 = 720. |
| 3 | Full / unsafe nearest shelter | Plan: 2,420 of 2,445 placed. Mark Assembly_North UNSAFE, cut Cricket_Ground to 500 | Every allocation explains itself, e.g. *"skipped Assembly_North (cost 210) is UNSAFE; Cricket_Ground (cost 280) is FULL; …chose Football_Ground (cost 510)"*. Unserved count rises with a reason; capacity is never exceeded. |
| 4 | Multiple incidents, different priorities | 12 incidents. Heap root = #12 (sev 5, pop 350), ahead of #1 (sev 5, pop 90) | Dispatch follows the priority rule; each unit is the nearest free one of the right type. |
| 5 | Insufficient resources | 4 fire, 4 medical and 4 rescue incidents, but only 2 units of each type | *"insufficient resources: all 2 FIRE_TRUCK units are busy"*. The incident stays pending; lower-priority incidents of other types are still served. |
| 6 | Disconnected location | Observatory's only road is BLOCKED | DFS reports the zone as DISCONNECTED; route query: *"unreachable (all roads to it are blocked)"*; 25 people unserved with the reason. |
| 7 | Change while running | Reopen the Observatory road; complete tasks; new sev-5 incident #13; revise #11 from sev 1 to 4 | Plan recomputed. #13 becomes the heap root at once and #11 moves up; freed units are reassigned from their new locations. |
| 8 | Invalid input | Duplicate ID, location 99, severity 9, population −4, capacity −5, non-shelter, non-existent road, status 7, completing a pending task, allocation 999, full/unsafe/non-shelter revision targets | Each one is rejected with a specific message, and state is unchanged (consistency check passes). |
| 9 | Cancel / revise allocation | Cancel allocation 1, move Lecture_Hall's 150 people into the space it freed | Occupancies, loads and unserved counts update correctly. |
| 10 | Undo (stack) | Undo the latest 2 updates | Reverted in LIFO order (road reopening first, then the capacity change), and the plan is recomputed. |
| 11 | AVL rotations and deletion | Insert incidents 14–18 (ascending), cancel 2, purge 6 closed records | RR cases and left rotations are printed; height stays at 5 for 18 keys; it is still a valid AVL after the deletions. |
| 12 | Interactive input validation | Letters or out-of-range numbers typed at the menu | Re-prompts instead of crashing. Malformed data files are rejected with the line at fault. |

## 8. Reports produced

Shown at the end of the demo, and available from menu option 25:

- evacuation assignments (route, cost, estimated minutes, reason);
- unserved people with the reason;
- shelter utilization (**merge sort**);
- resource status;
- critical bottlenecks — roads ranked by load/capacity, i.e. minutes needed to clear (**insertion sort**);
- ranked high-risk zones, with risk = 10·Σseverity² + unserved/10 + 50 if cut off (**quick sort**);
- incidents ranked by the priority rule (**quick sort**);
- event log (linked list) and the undo stack.

## 9. Limitations and possible improvements

- **O(V²) memory** for the adjacency matrix: 1,600 nodes already need 40 MB. A city-scale map would need an adjacency list, where Dijkstra with a heap runs in O((V+E) log V).
- **Greedy allocation**, building by building. It is not globally optimal: a later building may be sent far away because an earlier one took its nearest shelter. A min-cost-flow formulation would minimise the total evacuation time, but it is outside this course.
- **Static costs.** Road load does not feed back into the travel cost, so congestion caused by the plan itself is reported (bottleneck report) but not avoided. An iterative re-plan that raises the cost of loaded roads would fix this.
- **Starvation** of low-severity tasks. Aging (raising the priority with waiting time) could be added through `updatePriority`.
- **Undo** covers network and shelter updates only, not incident or allocation operations. Manual allocation edits are lost when any change triggers a re-plan (assumption A9).
- Single-threaded, discrete simulation. Travel and task durations are not simulated in real time.

## 10. CEP attribute mapping

| Attribute | Where it appears |
|---|---|
| P1 Depth of knowledge | Graph (BFS, DFS, Dijkstra), AVL, heap, queue, stack, linked lists, 3 sorts, recursion, measured complexity. |
| P2 Conflicting requirements | Safety vs speed (hazard penalty and last-resort pass), capacity vs nearest shelter (splitting), severity vs population (priority rule), urgency vs distance (dispatch). |
| P3 Depth of analysis | The cost and priority rules are defined and justified in Section 4. The alternatives (weighted priority sum, heap-based Dijkstra, adjacency list, min-cost flow) are evaluated against the chosen rules. |
| P6 Stakeholders | Evacuees (routes, times), administration (utilization, unserved), security (bottlenecks, disconnected zones), rescue teams (dispatch routes), facility managers (shelter capacity and safety). |
| P7 Interdependence | A road change re-runs routing, then allocation, then bottlenecks and risk. An incident changes the evacuation order and the dispatch queue. Completing a task moves the unit, which changes the next dispatch. |
