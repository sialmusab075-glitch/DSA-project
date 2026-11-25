// Name: Muhammad Musab Sial
// Reg No: 20255888
#include "Structures.h"

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
