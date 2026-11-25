// Name: Muhammad Musab Sial
// Reg No: 20255888
#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <iostream>
#include <string>
using namespace std;

const int INF = 1000000000;

const int TYPE_BUILDING = 0;
const int TYPE_GATE = 1;
const int TYPE_SHELTER = 2;
const int TYPE_JUNCTION = 3;

const int ROAD_NONE = 0;
const int ROAD_OPEN = 1;
const int ROAD_CONGESTED = 2;
const int ROAD_HAZARDOUS = 3;
const int ROAD_BLOCKED = 4;

const int RES_AMBULANCE = 0;
const int RES_FIRE_TRUCK = 1;
const int RES_RESCUE_TEAM = 2;

const int INC_FIRE = 0;
const int INC_GAS = 1;
const int INC_STRUCTURAL = 2;
const int INC_MEDICAL = 3;

const int TASK_PENDING = 0;
const int TASK_DISPATCHED = 1;
const int TASK_COMPLETED = 2;
const int TASK_CANCELLED = 3;

const int WALK_SPEED = 80;

struct Location {
    int id;
    string name;
    int type;
    int population;
    bool affected;
    int capacity;
    int occupancy;
    bool safe;
    int evacuated;
    string unservedReason;
};

struct Edge {
    int distance;
    int capacity;
    int status;
    int load;
};

struct Incident {
    int id;
    int location;
    int type;
    int severity;
    int population;
    int timeReceived;
    int status;
    int resourceIndex;
    bool used;
    string description;
    string note;
};

struct Resource {
    int id;
    string name;
    int type;
    int location;
    bool busy;
    int incidentId;
};

struct Allocation {
    int id;
    int building;
    int shelter;
    int people;
    int cost;
    int bottleneck;
    bool hazardous;
    int* path;
    int pathLength;
    string reason;
    Allocation* next;
};

struct SortItem {
    int index;
    double key1;
    double key2;
};

string locationTypeName(int type) {
    if (type == TYPE_BUILDING) return "BUILDING";
    if (type == TYPE_GATE) return "GATE";
    if (type == TYPE_SHELTER) return "SHELTER";
    if (type == TYPE_JUNCTION) return "JUNCTION";
    return "UNKNOWN";
}

string roadStatusName(int status) {
    if (status == ROAD_OPEN) return "OPEN";
    if (status == ROAD_CONGESTED) return "CONGESTED";
    if (status == ROAD_HAZARDOUS) return "HAZARDOUS";
    if (status == ROAD_BLOCKED) return "BLOCKED";
    return "NONE";
}

string resourceTypeName(int type) {
    if (type == RES_AMBULANCE) return "AMBULANCE";
    if (type == RES_FIRE_TRUCK) return "FIRE_TRUCK";
    if (type == RES_RESCUE_TEAM) return "RESCUE_TEAM";
    return "UNKNOWN";
}

string incidentTypeName(int type) {
    if (type == INC_FIRE) return "FIRE";
    if (type == INC_GAS) return "GAS_LEAK";
    if (type == INC_STRUCTURAL) return "STRUCTURAL";
    if (type == INC_MEDICAL) return "MEDICAL";
    return "UNKNOWN";
}

string taskStatusName(int status) {
    if (status == TASK_PENDING) return "PENDING";
    if (status == TASK_DISPATCHED) return "DISPATCHED";
    if (status == TASK_COMPLETED) return "COMPLETED";
    if (status == TASK_CANCELLED) return "CANCELLED";
    return "UNKNOWN";
}

int requiredResource(int incidentType) {
    if (incidentType == INC_FIRE) return RES_FIRE_TRUCK;
    if (incidentType == INC_MEDICAL) return RES_AMBULANCE;
    return RES_RESCUE_TEAM;
}

int parseLocationType(string s) {
    if (s == "BUILDING") return TYPE_BUILDING;
    if (s == "GATE") return TYPE_GATE;
    if (s == "SHELTER") return TYPE_SHELTER;
    if (s == "JUNCTION") return TYPE_JUNCTION;
    return -1;
}

int parseRoadStatus(string s) {
    if (s == "OPEN") return ROAD_OPEN;
    if (s == "CONGESTED") return ROAD_CONGESTED;
    if (s == "HAZARDOUS") return ROAD_HAZARDOUS;
    if (s == "BLOCKED") return ROAD_BLOCKED;
    return -1;
}

int parseResourceType(string s) {
    if (s == "AMBULANCE") return RES_AMBULANCE;
    if (s == "FIRE_TRUCK") return RES_FIRE_TRUCK;
    if (s == "RESCUE_TEAM") return RES_RESCUE_TEAM;
    return -1;
}

int parseIncidentType(string s) {
    if (s == "FIRE") return INC_FIRE;
    if (s == "GAS_LEAK") return INC_GAS;
    if (s == "STRUCTURAL") return INC_STRUCTURAL;
    if (s == "MEDICAL") return INC_MEDICAL;
    return -1;
}

#endif
