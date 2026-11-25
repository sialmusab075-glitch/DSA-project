// Name: Muhammad Musab Sial
// Reg No: 20255888
#include "EvacuationPlanner.h"

bool EvacuationPlanner::setRoadStatus(int fromId, int toId, int status, bool recordUndo) {
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

bool EvacuationPlanner::setShelterCapacity(int id, int capacity, bool recordUndo) {
    int s = locationIndex.search(id);
    if (s == -1 || locs[s].type != TYPE_SHELTER) {
        cout << "  Rejected: " << id << " is not a shelter." << endl;
        return false;
    }
    if (capacity < 0 || capacity > MAX_PEOPLE) {
        cout << "  Rejected: capacity must be between 0 and " << MAX_PEOPLE << "." << endl;
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

bool EvacuationPlanner::setShelterSafety(int id, bool safe, bool recordUndo) {
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

bool EvacuationPlanner::setAffected(int id, bool affected) {
    int b = locationIndex.search(id);
    if (b == -1 || locs[b].type != TYPE_BUILDING) {
        cout << "  Rejected: " << id << " is not a building." << endl;
        return false;
    }
    if (locs[b].affected == affected) {
        cout << "  No change: " << locs[b].name << " is already " << (affected ? "affected." : "clear.") << endl;
        return false;
    }
    locs[b].affected = affected;
    if (!affected) {
        locs[b].held = 0;
    }
    clock++;
    addLog("Building " + locs[b].name + (affected ? " marked affected" : " cleared"));
    cout << "  " << locs[b].name << (affected ? " is now affected." : " is no longer affected.") << endl;
    replanIfActive();
    return true;
}

bool EvacuationPlanner::setWindow(int minutes, bool recordUndo) {
    if (minutes < 1 || minutes > 240) {
        cout << "  Rejected: the evacuation window must be 1 to 240 minutes." << endl;
        return false;
    }
    if (minutes == window) {
        cout << "  No change: the window is already " << minutes << " minutes." << endl;
        return false;
    }
    string text = "Evacuation window: " + toText(window) + " -> " + toText(minutes) +
                  " min (each road carries at most capacity x window people)";
    if (recordUndo) {
        UndoRecord r;
        r.type = UNDO_WINDOW;
        r.a = -1;
        r.b = -1;
        r.oldValue = window;
        r.description = text;
        undoStack.push(r);
    }
    window = minutes;
    clock++;
    addLog(text);
    cout << "  " << text << endl;
    replanIfActive();
    return true;
}

bool EvacuationPlanner::undoLast() {
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
    if (r.type == UNDO_WINDOW) {
        return setWindow(r.oldValue, false);
    }
    return setShelterSafety(locs[r.a].id, r.oldValue == 1, false);
}

void EvacuationPlanner::printUndoStack() {
    cout << "  Undo stack (" << undoStack.size() << " records, LIFO):" << endl;
    undoStack.print();
}
