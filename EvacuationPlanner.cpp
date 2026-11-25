// Name: Muhammad Musab Sial
// Reg No: 20255888
#include "EvacuationPlanner.h"

string EvacuationPlanner::toText(int value) {
    stringstream ss;
    ss << value;
    return ss.str();
}

bool EvacuationPlanner::readToken(ifstream& in, string& token) {
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

bool EvacuationPlanner::readNumber(ifstream& in, int& value) {
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

bool EvacuationPlanner::expectHeader(ifstream& in, string name, int& count) {
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

void EvacuationPlanner::addLog(string text) {
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

EvacuationPlanner::EvacuationPlanner() : locationIndex("LocationAVL"), incidentIndex("IncidentAVL") {
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
    window = DEFAULT_WINDOW;
    logHead = NULL;
    logTail = NULL;
    clock = 0;
    dist = NULL;
    parent = NULL;
    path = NULL;
}

EvacuationPlanner::~EvacuationPlanner() {
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

bool EvacuationPlanner::load(string filename) {
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
        if (locs[i].type == -1 || locs[i].id <= 0 || amount < 0 || amount > MAX_PEOPLE || (affected != 0 && affected != 1)) {
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
        locs[i].held = 0;
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

int EvacuationPlanner::locationIndexOf(int id) {
    return locationIndex.search(id);
}

void EvacuationPlanner::printSummary() {
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

void EvacuationPlanner::printLocations() {
    cout << "  " << setw(4) << "ID" << setw(22) << "Name" << setw(10) << "Type" << setw(8) << "People"
         << setw(10) << "Capacity" << setw(10) << "Affected" << setw(8) << "Safe" << endl;
    for (int i = 0; i < numLocs; i++) {
        cout << "  " << setw(4) << locs[i].id << setw(22) << locs[i].name << setw(10)
             << locationTypeName(locs[i].type) << setw(8) << locs[i].population << setw(10)
             << locs[i].capacity << setw(10) << (locs[i].affected ? "yes" : "-") << setw(8)
             << (locs[i].type == TYPE_SHELTER ? (locs[i].safe ? "yes" : "NO") : "-") << endl;
    }
}

void EvacuationPlanner::printRoads() {
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

void EvacuationPlanner::showLocationIndex() {
    cout << "  Location AVL index: " << locationIndex.size() << " keys, height " << locationIndex.treeHeight()
         << ", valid AVL: " << (locationIndex.isValidAVL() ? "yes" : "NO") << endl;
    locationIndex.printTraversals();
    cout << "  Tree shape (rotate page 90 degrees clockwise, root on the left):" << endl;
    locationIndex.printShape();
}

void EvacuationPlanner::showIncidentIndex() {
    cout << "  Incident AVL index: " << incidentIndex.size() << " keys, height " << incidentIndex.treeHeight()
         << ", rotations so far " << incidentIndex.rotationCount() << ", valid AVL: "
         << (incidentIndex.isValidAVL() ? "yes" : "NO") << endl;
    incidentIndex.printTraversals();
    incidentIndex.printShape();
}

void EvacuationPlanner::setShowRotations(bool show) {
    incidentIndex.setShowRotations(show);
    locationIndex.setShowRotations(show);
}

bool EvacuationPlanner::searchLocation(int id) {
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

void EvacuationPlanner::printEventLog() {
    LogNode* current = logHead;
    while (current != NULL) {
        cout << "  t=" << setw(3) << current->time << "  " << current->text << endl;
        current = current->next;
    }
}

int EvacuationPlanner::getClock() {
    return clock;
}

int EvacuationPlanner::allocationIdAt(int position) {
    Allocation* a = allocHead;
    for (int i = 0; i < position && a != NULL; i++) {
        a = a->next;
    }
    if (a == NULL) {
        return -1;
    }
    return a->id;
}

int EvacuationPlanner::allocationIdFor(int buildingId, int shelterId) {
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
