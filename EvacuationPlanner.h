// Name: Muhammad Musab Sial
// Reg No: 20255888
#ifndef EVACUATIONPLANNER_H
#define EVACUATIONPLANNER_H

#include <fstream>
#include <iomanip>
#include <sstream>
#include "Structures.h"
#include "QueueStack.h"
#include "AVLTree.h"
#include "IncidentHeap.h"
#include "Sorting.h"
#include "CampusGraph.h"

struct LogNode {
    int time;
    string text;
    LogNode* next;
};

class EvacuationPlanner {
private:
    CampusGraph* graph;
    Location* locs;
    int numLocs;
    AVLTree locationIndex;
    Incident* incidents;
    int incidentCap;
    AVLTree incidentIndex;
    IncidentHeap* heap;
    Resource* resources;
    int numResources;
    Allocation* allocHead;
    Allocation* allocTail;
    int nextAllocId;
    bool planActive;
    int window;
    UndoStack undoStack;
    LogNode* logHead;
    LogNode* logTail;
    int clock;
    int* dist;
    int* parent;
    int* path;

    string toText(int value);
    bool readToken(ifstream& in, string& token);
    bool readNumber(ifstream& in, int& value);
    bool expectHeader(ifstream& in, string name, int& count);
    void addLog(string text);
    string pathToString(int* p, int length);
    int pathBottleneck(int* p, int length);
    bool pathUsesHazard(int* p, int length);
    int incidentScore(int locIndex);
    int unservedAt(int i);
    bool shelterReachable(int i);
    int pathResidual(int* p, int length, int& limitFrom, int& limitTo);
    void applyLoads(Allocation* a, int sign);
    Allocation* addAllocation(int building, int shelter, int people, int cost, int* p, int length, string reason);
    void releaseAllocation(Allocation* a);
    void clearAllocations();
    int estimatedMinutes(Allocation* a);
    string unservedReasonFor(int b);
    void evacuateBuilding(int b, bool verbose);
    bool placePinned(int b, int s, int people, string& why);
    void replanIfActive();
    int agedPriority(Incident& inc);
    void applyAging();
    int findIncidentSlot(int id);
    int findResource(int id);
    int findBestResource(int slot, string& reason, int& bestCost);
    void printIncidentRow(int slot);

public:
    EvacuationPlanner();
    ~EvacuationPlanner();
    bool load(string filename);
    int locationIndexOf(int id);
    void printSummary();
    void printLocations();
    void printRoads();
    void showLocationIndex();
    void showIncidentIndex();
    void setShowRotations(bool show);
    bool searchLocation(int id);
    void traversalReport(int fromId);
    void connectivityReport();
    bool routeToShelter(int fromId);
    void routesForAffected();
    bool routeBetween(int fromId, int toId);
    void planEvacuation(bool verbose);
    bool setRoadStatus(int fromId, int toId, int status, bool recordUndo);
    bool setShelterCapacity(int id, int capacity, bool recordUndo);
    bool setShelterSafety(int id, bool safe, bool recordUndo);
    bool setAffected(int id, bool affected);
    bool setWindow(int minutes, bool recordUndo);
    bool undoLast();
    void printUndoStack();
    bool registerIncident(int id, int locId, int type, int severity, int population, string description,
                          int time, bool announce);
    bool reviseIncident(int id, int severity, int population);
    bool cancelIncident(int id);
    bool dispatchNext();
    int dispatchAll();
    bool completeTask(int id);
    int purgeClosedIncidents();
    bool searchIncident(int id);
    bool cancelAllocation(int allocId);
    bool releaseHeld(int buildingId);
    bool reviseAllocation(int allocId, int shelterId);
    void showHeap();
    void reportAllocations();
    void reportUnserved();
    void reportShelterUtilization();
    void reportResources();
    void reportBottlenecks();
    void reportHighRisk();
    void reportIncidents();
    void printEventLog();
    bool consistencyCheck();
    int getClock();
    int allocationIdAt(int position);
    int allocationIdFor(int buildingId, int shelterId);
};

#endif
