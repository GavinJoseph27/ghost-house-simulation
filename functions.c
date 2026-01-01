#include "defs.h"
#include "helpers.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Initialize ghost fields and place ghost in a random room
void ghost_init(struct Ghost* ghost, struct House* house) {
    ghost->id = DEFAULT_GHOST_ID;

    // Pick a random ghost type
    const enum GhostType* types;
    int count = get_all_ghost_types(&types);
    int index = rand_int_threadsafe(0, count);
    ghost->ghostType = types[index];

    // Pick a random starting room
    int index2 = rand_int_threadsafe(0, house->room_count);
    ghost->hidden = &house->rooms[index2];

    ghost->boredom = 0;
    ghost->exitSim = false;

    // Mark ghost as present in room
    ghost->hidden->ghostRoom = ghost;

    log_ghost_init(ghost->id, ghost->hidden->name, ghost->ghostType);
}

// Add a new hunter to the house and assign a random device
void hunter_add(struct House* house, const char* name, int id) {
    // Expand hunter array if necessary
    if (house->hunterCount == house->hunterCapacity){
        house->hunterCapacity = (house->hunterCapacity == 0 ? 1 : house->hunterCapacity * 2);
        house->hunter = realloc(house->hunter, house->hunterCapacity * sizeof(struct Hunter));
    }

    struct Hunter* hunt = &house->hunter[house->hunterCount];
    
    // Basic hunter info
    strncpy(hunt->name, name, MAX_HUNTER_NAME - 1);
    hunt->name[MAX_HUNTER_NAME - 1] = '\0';
    hunt->current = house->starting_room;
    hunt->id = id;
    hunt->home = house;

    // Assign random investigation device
    const enum EvidenceType* devices;
    int devCount = get_all_evidence_types(&devices);
    int index = rand_int_threadsafe(0, devCount);
    hunt->currentDevice = devices[index];

    // Share global case file
    hunt->file = &house->fileCase;
    
    // Init breadcrumb stack
    hunt->path.top = NULL;

    hunt->fear = 0;
    hunt->boredom = 0;

    hunt->exitHouse = false;
    hunt->whyExit = LR_EVIDENCE;

    // Add hunter to starting room
    struct Room* room = hunt->current;
    sem_wait(&room->mutex);
    if (room->numHunters < MAX_ROOM_OCCUPANCY) {
        room->hunters[room->numHunters++] = hunt;
    }
    sem_post(&room->mutex);

    house->hunterCount++;

    log_hunter_init(hunt->id, room->name, hunt->name, hunt->currentDevice);

}

// Add hunter to starting room
void roomstack_push(struct RoomStack* stack, struct Room* room) {
    struct RoomNode* newNode = malloc(sizeof(struct RoomNode)); // // allocate new node
    newNode->room = room; // store room
    newNode->next = stack->top; // insert at top
    stack->top = newNode; // update head
}

// Pop room from breadcrumb stack
struct Room* roomstack_pop(struct RoomStack* stack) {
    if (stack->top == NULL) return NULL;
    struct RoomNode* handleNode = stack->top; // get top node
    struct Room* room = handleNode->room; // extract room
    stack->top = handleNode->next; // move head
    free(handleNode); // free node
    return room; // return popped room
}

// Remove all rooms from breadcrumb stack
void roomstack_clear(struct RoomStack* stack) {
    while (stack->top != NULL) roomstack_pop(stack);  // pop until empty
}

// Main hunter behavior loop (movement, fear, boredom, evidence collection)
void *hunter_thread(void* arg) {
    struct Hunter* hunt = (struct Hunter*)arg;
    struct House* house  = hunt->home;
    struct Ghost* ghost  = &house->ghost;
    struct CaseFile* file = &house->fileCase;

    while (!hunt->exitHouse) {

        // Evidence Collection
        if (hunt->current->evidence != 0) {
            sem_wait(&hunt->current->mutex);
            EvidenceByte mask = hunt->current->evidence;
            hunt->current->evidence = 0;
            sem_post(&hunt->current->mutex);

            enum EvidenceType ev = (enum EvidenceType)mask;

            log_evidence(hunt->id, hunt->boredom, hunt->fear,
                         hunt->current->name, ev);

            // Update shared case file
            sem_wait(&file->mutex);
            file->collected |= mask;
            file->solved = evidence_has_three_unique(file->collected);
            sem_post(&file->mutex);
        }

        // Exit due to fear
        if (hunt->fear >= HUNTER_FEAR_MAX) {
            hunt->exitHouse = true;
            hunt->whyExit = LR_AFRAID;
            log_exit(hunt->id, hunt->boredom, hunt->fear,
                     hunt->current->name, hunt->currentDevice, LR_AFRAID);
            break;
        }

        // Exit due to boredom
        if (hunt->boredom >= ENTITY_BOREDOM_MAX) {
            hunt->exitHouse = true;
            hunt->whyExit = LR_BORED;
            log_exit(hunt->id, hunt->boredom, hunt->fear,
                     hunt->current->name, hunt->currentDevice, LR_BORED);
            break;
        }

        // Check if case is solved
        sem_wait(&file->mutex);
        bool solved = file->solved;
        sem_post(&file->mutex);

        if (solved) {
            // Return to Van if case is solved

            log_return_to_van(hunt->id, hunt->boredom, hunt->fear,
                              hunt->current->name, hunt->currentDevice, true);

            // Follow breadcrumb trail back to Van
            while (hunt->current && strcmp(hunt->current->name, "Van") != 0) {

                struct Room* from = hunt->current;
                struct Room* to   = roomstack_pop(&hunt->path);

                if (!to) break; 

                log_move(hunt->id, hunt->boredom, hunt->fear,
                         from->name, to->name, hunt->currentDevice);

                hunt->current = to;
            }

            // Arrived at van
            log_return_to_van(hunt->id, hunt->boredom, hunt->fear,
                              "Van", hunt->currentDevice, false);

            hunt->exitHouse = true;
            hunt->whyExit = LR_EVIDENCE;
            break;
        }

        // Check if ghost is in the room 
        sem_wait(&hunt->current->mutex);
        bool ghost_here = (hunt->current->ghostRoom != NULL);
        sem_post(&hunt->current->mutex);

        // Fear rises if ghost present, boredom rises otherwise
        if (ghost_here) {
            hunt->boredom = 0;
            hunt->fear++;
        } else {
            hunt->boredom++;
        }

        // Exit room with corrent ghost match
        if (hunt->current->exitRoom) {

            roomstack_clear(&hunt->path); // Clear breadcrumb path

            // Check if collected evidence matches ghost type
            sem_wait(&file->mutex);
            bool full_match = ((file->collected & ghost->ghostType) == ghost->ghostType);
            sem_post(&file->mutex);

            if (full_match) {
                // Remove hunter from room list
                struct Room* r = hunt->current;

                sem_wait(&r->mutex);
                for (int i = 0; i < r->numHunters; i++) {
                    if (r->hunters[i] == hunt) {
                        for (int j = i; j < r->numHunters - 1; j++)
                            r->hunters[j] = r->hunters[j + 1];

                        r->hunters[r->numHunters - 1] = NULL;
                        r->numHunters--;
                        break;
                    }
                }
                sem_post(&r->mutex);

                log_exit(hunt->id, hunt->boredom, hunt->fear,
                         r->name, hunt->currentDevice, LR_EVIDENCE);

                hunt->current = NULL;
                hunt->exitHouse = true;
                break;
            }
        }

        //Random movement to connected room
        struct Room* cur = hunt->current;
        int count = cur->connectionCount;

        if (count > 0) {

            // Pick connected room
            int index = rand_int_threadsafe(0, count);
            struct Room* nextRoom = cur->connected[index];

            // Remove hunter from current room
            sem_wait(&cur->mutex);
            for (int i = 0; i < cur->numHunters; i++) {
                if (cur->hunters[i] == hunt) {
                    for (int j = i; j < cur->numHunters - 1; j++)
                        cur->hunters[j] = cur->hunters[j + 1];

                    cur->hunters[cur->numHunters - 1] = NULL;
                    cur->numHunters--;
                    break;
                }
            }
            sem_post(&cur->mutex);

            roomstack_push(&hunt->path, cur); // Save breadcrumb

            log_move(hunt->id, hunt->boredom, hunt->fear,
                     cur->name, nextRoom->name, hunt->currentDevice);

            // Add hunter to next room
            sem_wait(&nextRoom->mutex);
            if (nextRoom->numHunters < MAX_ROOM_OCCUPANCY) {
                nextRoom->hunters[nextRoom->numHunters++] = hunt;
            }
            sem_post(&nextRoom->mutex);

            hunt->current = nextRoom;
        }
    }

    return NULL;
}

// Main ghost behavior loop (movement, evidence, exit)
void *ghost_thread(void* arg) {
    struct Ghost* ghost = (struct Ghost*)arg;
    struct Room* current = ghost->hidden;

    while (!ghost->exitSim) {

        // Exit if too bored
        if (ghost->boredom >= ENTITY_BOREDOM_MAX) {
            ghost->exitSim = true;
            log_ghost_exit(ghost->id, ghost->boredom, current->name);
            break;
        }

        // Randomly drop evidence
        if (rand_int_threadsafe(0, 6) == 0) {
            const enum EvidenceType* devices;
            int dcount = get_all_evidence_types(&devices);
            int idx = rand_int_threadsafe(0, dcount);
            enum EvidenceType ev = devices[idx];

            sem_wait(&current->mutex);
            current->evidence |= ev;
            sem_post(&current->mutex);

            log_ghost_evidence(ghost->id, ghost->boredom, current->name, ev);
        }

        // Move to a connected room
        int count = current->connectionCount;
        if (count > 0) {
            int index = rand_int_threadsafe(0, count);
            struct Room* next = current->connected[index];

            log_ghost_move(ghost->id, ghost->boredom,
                           current->name, next->name);

            // Leave current room
            sem_wait(&current->mutex);
            current->ghostRoom = NULL;
            sem_post(&current->mutex);

            // Enter next room
            sem_wait(&next->mutex);
            next->ghostRoom = ghost;
            sem_post(&next->mutex);

            ghost->hidden = next;
            current = next;
        } else {
            // No movement possible
            log_ghost_idle(ghost->id, ghost->boredom, current->name);
        }

        ghost->boredom++;
    }

    return NULL;
}

// Initialize room fields
void room_init(struct Room* room, const char* name, bool is_exit) {
    strcpy(room->name, name);
    
    // Clear connections
    room->connectionCount = 0;
    for(int i = 0; i < MAX_CONNECTIONS; i++){
        room->connected[i] = NULL;
    }

    room->ghostRoom = NULL;

    // Clear hunter occupancy
    room->numHunters = 0;
    for(int i = 0; i < MAX_ROOM_OCCUPANCY; i++){
        room->hunters[i] = NULL;
    }

    room->exitRoom = is_exit;
    room->evidence = 0;

    // Mutex for room
    sem_init(&room->mutex, 0, 1);
}

// Create bidirectional connection between rooms
void room_connect(struct Room* a, struct Room* b) {
    a->connected[a->connectionCount++] = b;
    b->connected[b->connectionCount++] = a;
}