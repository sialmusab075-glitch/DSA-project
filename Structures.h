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
const int DEFAULT_WINDOW = 10;
const int AGING_INTERVAL = 30;
const int AGING_MAX_BOOST = 2;
const int MAX_PEOPLE = 100000;
const int MAX_DISTANCE = 100000;
const int MAX_ROAD_CAPACITY = 10000;

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
    int held;
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
    int priority;
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
    bool pinned;
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

string locationTypeName(int type);
string roadStatusName(int status);
string resourceTypeName(int type);
string incidentTypeName(int type);
string taskStatusName(int status);
int requiredResource(int incidentType);
int parseLocationType(string s);
int parseRoadStatus(string s);
int parseResourceType(string s);
int parseIncidentType(string s);

#endif
