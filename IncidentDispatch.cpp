// Name: Muhammad Musab Sial
// Reg No: 20255888
#include "EvacuationPlanner.h"

int EvacuationPlanner::incidentScore(int locIndex) {
    int score = 0;
    for (int i = 0; i < incidentCap; i++) {
        if (incidents[i].used && incidents[i].location == locIndex &&
            (incidents[i].status == TASK_PENDING || incidents[i].status == TASK_DISPATCHED)) {
            score += incidents[i].severity * incidents[i].severity * 10;
        }
    }
    return score;
}

int EvacuationPlanner::agedPriority(Incident& inc) {
    int boost = (clock - inc.timeReceived) / AGING_INTERVAL;
    if (boost < 0) {
        boost = 0;
    }
    if (boost > AGING_MAX_BOOST) {
        boost = AGING_MAX_BOOST;
    }
    int limit = 4;
    if (inc.severity == 5) {
        limit = 5;
    }
    int value = inc.severity + boost;
    if (value > limit) {
        value = limit;
    }
    if (value < inc.severity) {
        value = inc.severity;
    }
    return value;
}

void EvacuationPlanner::applyAging() {
    bool changed = false;
    for (int i = 0; i < incidentCap; i++) {
        if (incidents[i].used && incidents[i].status == TASK_PENDING) {
            int value = agedPriority(incidents[i]);
            if (value != incidents[i].priority) {
                cout << "  Aging: incident " << incidents[i].id << " has waited " << clock - incidents[i].timeReceived
                     << " min, priority " << incidents[i].priority << " -> " << value << endl;
                incidents[i].priority = value;
                changed = true;
            }
        }
    }
    if (changed) {
        heap->rebuild();
    }
}

int EvacuationPlanner::findIncidentSlot(int id) {
    return incidentIndex.search(id);
}

int EvacuationPlanner::findResource(int id) {
    for (int i = 0; i < numResources; i++) {
        if (resources[i].id == id) {
            return i;
        }
    }
    return -1;
}

int EvacuationPlanner::findBestResource(int slot, string& reason, int& bestCost) {
    Incident& inc = incidents[slot];
    int needed = requiredResource(inc.type);
    graph->dijkstra(inc.location, dist, parent, true, 0);
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

void EvacuationPlanner::printIncidentRow(int slot) {
    Incident& inc = incidents[slot];
    cout << "  " << setw(4) << inc.id << setw(18) << locs[inc.location].name << setw(12)
         << incidentTypeName(inc.type) << setw(5) << inc.severity << setw(5) << inc.priority << setw(7) << inc.population
         << setw(6) << inc.timeReceived << setw(12) << taskStatusName(inc.status);
    if (inc.status == TASK_DISPATCHED && inc.resourceIndex >= 0) {
        cout << "  " << resources[inc.resourceIndex].name;
    } else if (inc.note != "") {
        cout << "  " << inc.note;
    }
    cout << endl;
}

bool EvacuationPlanner::registerIncident(int id, int locId, int type, int severity, int population, string description,
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
    if (population < 0 || population > MAX_PEOPLE) {
        cout << "  Rejected: affected population must be between 0 and " << MAX_PEOPLE << "." << endl;
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
    inc.priority = agedPriority(inc);
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

bool EvacuationPlanner::reviseIncident(int id, int severity, int population) {
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
    if (severity < 1 || severity > 5 || population < 0 || population > MAX_PEOPLE) {
        cout << "  Rejected: severity must be 1-5 and population 0-" << MAX_PEOPLE << "." << endl;
        return false;
    }
    cout << "  Incident " << id << " revised: severity " << incidents[slot].severity << " -> " << severity
         << ", population " << incidents[slot].population << " -> " << population << endl;
    incidents[slot].severity = severity;
    incidents[slot].population = population;
    incidents[slot].priority = agedPriority(incidents[slot]);
    heap->updatePriority(slot);
    addLog("Incident " + toText(id) + " priority revised");
    return true;
}

bool EvacuationPlanner::cancelIncident(int id) {
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

bool EvacuationPlanner::dispatchNext() {
    applyAging();
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

int EvacuationPlanner::dispatchAll() {
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

bool EvacuationPlanner::completeTask(int id) {
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

int EvacuationPlanner::purgeClosedIncidents() {
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

bool EvacuationPlanner::searchIncident(int id) {
    int slot = findIncidentSlot(id);
    if (slot == -1) {
        cout << "  Incident " << id << " not found." << endl;
        return false;
    }
    printIncidentRow(slot);
    return true;
}

void EvacuationPlanner::showHeap() {
    applyAging();
    int* contents = new int[incidentCap];
    int n = heap->copyContents(contents);
    cout << "  Binary max-heap array (" << n << " pending), valid heap: " << (heap->isValidHeap() ? "yes" : "NO")
         << endl << "   ";
    for (int i = 0; i < n; i++) {
        cout << " [" << i << "]#" << incidents[contents[i]].id << "(pr" << incidents[contents[i]].priority << ",p"
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
