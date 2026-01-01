#ifndef DEFS_H
#define DEFS_H

#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>

#define MAX_ROOM_NAME 64
#define MAX_HUNTER_NAME 64
#define MAX_ROOMS 24
#define MAX_ROOM_OCCUPANCY 8
#define MAX_CONNECTIONS 8
#define ENTITY_BOREDOM_MAX 15
#define HUNTER_FEAR_MAX 15
#define DEFAULT_GHOST_ID 68057

//Evidence stored as bitmasks
typedef unsigned char EvidenceByte; 

// Reasons a hunter leaves the house
enum LogReason {
    LR_EVIDENCE = 0,
    LR_BORED = 1,
    LR_AFRAID = 2
};

// Individual evidence types
enum EvidenceType {
    EV_EMF          = 1 << 0,
    EV_ORBS         = 1 << 1,
    EV_RADIO        = 1 << 2,
    EV_TEMPERATURE  = 1 << 3,
    EV_FINGERPRINTS = 1 << 4,
    EV_WRITING      = 1 << 5,
    EV_INFRARED     = 1 << 6,
};

// Each ghost type is defined by 3 required evidence flags
enum GhostType {
    GH_POLTERGEIST  = EV_FINGERPRINTS | EV_TEMPERATURE | EV_WRITING,
    GH_THE_MIMIC    = EV_FINGERPRINTS | EV_TEMPERATURE | EV_RADIO,
    GH_HANTU        = EV_FINGERPRINTS | EV_TEMPERATURE | EV_ORBS,
    GH_JINN         = EV_FINGERPRINTS | EV_TEMPERATURE | EV_EMF,
    GH_PHANTOM      = EV_FINGERPRINTS | EV_INFRARED    | EV_RADIO,
    GH_BANSHEE      = EV_FINGERPRINTS | EV_INFRARED    | EV_ORBS,
    GH_GORYO        = EV_FINGERPRINTS | EV_INFRARED    | EV_EMF,
    GH_BULLIES      = EV_FINGERPRINTS | EV_WRITING     | EV_RADIO,
    GH_MYLING       = EV_FINGERPRINTS | EV_WRITING     | EV_EMF,
    GH_OBAKE        = EV_FINGERPRINTS | EV_ORBS        | EV_EMF,
    GH_YUREI        = EV_TEMPERATURE  | EV_INFRARED    | EV_ORBS,
    GH_ONI          = EV_TEMPERATURE  | EV_INFRARED    | EV_EMF,
    GH_MOROI        = EV_TEMPERATURE  | EV_WRITING     | EV_RADIO,
    GH_REVENANT     = EV_TEMPERATURE  | EV_WRITING     | EV_ORBS,
    GH_SHADE        = EV_TEMPERATURE  | EV_WRITING     | EV_EMF,
    GH_ONRYO        = EV_TEMPERATURE  | EV_RADIO       | EV_ORBS,
    GH_THE_TWINS    = EV_TEMPERATURE  | EV_RADIO       | EV_EMF,
    GH_DEOGEN       = EV_INFRARED     | EV_WRITING     | EV_RADIO,
    GH_THAYE        = EV_INFRARED     | EV_WRITING     | EV_ORBS,
    GH_YOKAI        = EV_INFRARED     | EV_RADIO       | EV_ORBS,
    GH_WRAITH       = EV_INFRARED     | EV_RADIO       | EV_EMF,
    GH_RAIJU        = EV_INFRARED     | EV_ORBS        | EV_EMF,
    GH_MARE         = EV_WRITING      | EV_RADIO       | EV_ORBS,
    GH_SPIRIT       = EV_WRITING      | EV_RADIO       | EV_EMF,
};

// Shared evidence gathered by all hunters
struct CaseFile {
    EvidenceByte collected; // Union of all of the evidence bits collected between all hunters
    bool         solved;    // True when >=3 unique bits set
    sem_t        mutex;     // Used for synchronizing both fields when multithreading
};

// Room data structure
struct Room {
    char name[MAX_ROOM_NAME]; //Room name

    struct Room* connected[MAX_CONNECTIONS]; // Adjacent rooms
    int connectionCount; // Number of connections

    struct House* home; // Back pointer to the house

    struct Ghost* ghostRoom;// Ghost if present

    struct Hunter* hunters[MAX_ROOM_OCCUPANCY]; // Hunters inside room
    int numHunters; // Number of hunters inside

    bool exitRoom; // True if this room is the exit

    EvidenceByte evidence; // Evidence placed here

    sem_t mutex; // Controls access to room data
};

// Linked-list node for stack of rooms
struct RoomNode{
    struct Room* room; // Room stored
    struct RoomNode* next; // Next node
};

// Stack of previously visited rooms
struct RoomStack{
    struct RoomNode* top; // Head of stack
};

// Hunter state
struct Hunter{
    char name[MAX_HUNTER_NAME]; // Hunter name
    int id; // Hunter ID

    struct House* home; // Pointer to house
    
    struct Room* current; // Current room
    
    struct CaseFile* file; // Shared case file
    enum EvidenceType currentDevice; // Device hunter holds
    
    struct RoomStack path; // Breadcrumb stack
    
    int fear; // Fear counter
    int boredom; // Boredom counter
    enum LogReason whyExit; // Exit reason
    bool exitHouse; // True when leaving
};

// Ghost state
struct Ghost {
    int id; // Provided ghost ID
    enum GhostType ghostType; // Actual ghost type

    struct Room* hidden; // Current room

    int boredom; // Boredom counter
    bool exitSim; // True when ghost is done
};

// Full house structure
struct House {
    struct Room* starting_room; // First room (Van)
    
    struct Room rooms[MAX_ROOMS]; // Room list
    int room_count; // Total count

    struct Hunter* hunter; // Dynamic hunter array
    int hunterCount; // Number of hunters
    int hunterCapacity; // Capacity of hunter array

    struct CaseFile fileCase; // Shared case file

    struct Ghost ghost; // The ghost
};

// Function prototypes
void ghost_init(struct Ghost* ghost, struct House* house); // Initialize ghost state
void hunter_add(struct House* house, const char* name, int id); // Add a hunter to house
void roomstack_push(struct RoomStack* stack, struct Room* room); // Push a room onto hunter breadcrumb stack
struct Room* roomstack_pop(struct RoomStack* stack); // Pop a room from breadcrumb stack
void roomstack_clear(struct RoomStack* stack); // Clear entire breadcrumb stack
void *hunter_thread(void *arg); // Hunter thread logic
void *ghost_thread(void *arg); // Ghost thread logic
void room_init(struct Room* room, const char* name, bool is_exit); // Initialize a room
void room_connect(struct Room* a, struct Room* b); // Bidirectional connection

#endif // DEFS_H
