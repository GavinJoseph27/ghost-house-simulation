#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "helpers.h"

int main() {

    struct House house;
    memset(&house, 0, sizeof(house)); // Clear all fields in House

    house_populate_rooms(&house); // Build all rooms and map layout

    sem_init(&house.fileCase.mutex, 0, 1); // Init CaseFile mutex
    house.fileCase.collected = 0; // No evidence collected yet
    house.fileCase.solved = false; // Case not solved

    house.hunter = NULL; // Dynamic array starts empty
    house.hunterCount = 0;
    house.hunterCapacity = 0;

    ghost_init(&house.ghost, &house); // Randomize ghost type + start room

    char name[MAX_HUNTER_NAME];
    int id;

    printf(
        "\033[31m"
        "=====================================\n"
        "||   GHOST HOUSE INVESTIGATION!    ||\n"
        "=====================================\n"
        "\033[0m"
        "\n"
    );

    printf("Enter hunter name (max 63 characters) or 'done' to finish: ");

    // User input loop for hunters
    while (scanf("%63s", name) == 1 && strcmp(name, "done") != 0) {
        printf("Enter hunter ID: ");
        scanf("%d", &id);
        hunter_add(&house, name, id); // Add hunter to House
        printf("\nEnter next hunter name (max 63 characters) or 'done' to finish: ");
    }

    // Thread creation
    pthread_t ghostThread;
    pthread_create(&ghostThread, NULL, ghost_thread, &house.ghost);
    
    // Create one thread per hunter
    pthread_t* hunterThreads = malloc(sizeof(pthread_t) * house.hunterCount);

    for (int i = 0; i < house.hunterCount; i++) {
        pthread_create(&hunterThreads[i], NULL, hunter_thread, &house.hunter[i]);
    }

    // Thread join
    for (int i = 0; i < house.hunterCount; i++) {
        pthread_join(hunterThreads[i], NULL); // Wait for hunter to finish
        roomstack_clear(&house.hunter[i].path); // Free breadcrumb stack
    }

    // Wait for ghost thread
    pthread_join(ghostThread, NULL);

    // Final output
    printf(
        "\n"
        "\033[31m"
        "=========================\n"
        "||   FINAL RESULTS!    ||\n"
        "=========================\n"
        "\033[0m"
        "\n"
    );

    // Print why each hunter exited
    for (int i = 0; i < house.hunterCount; i++) {
        struct Hunter* h = &house.hunter[i];

        const char* reason = exit_reason_to_string(h->whyExit);

        printf("[✗] Hunter %s (ID %d) exited because of [%s] (bored=%d fear=%d).\n",
               h->name, h->id, reason, h->boredom, h->fear);
    }

    // Evidence Checklist
    printf("\nShared Case File Checklist:\n");

    EvidenceByte mask = house.fileCase.collected;

    printf(" - [%s] emf\n",      (mask & EV_EMF)          ? "\033[32m✔\033[0m" : " ");
    printf(" - [%s] orbs\n",     (mask & EV_ORBS)         ? "\033[32m✔\033[0m" : " ");
    printf(" - [%s] radio\n",    (mask & EV_RADIO)        ? "\033[32m✔\033[0m" : " ");
    printf(" - [%s] temp\n",     (mask & EV_TEMPERATURE)  ? "\033[32m✔\033[0m" : " ");
    printf(" - [%s] prints\n",   (mask & EV_FINGERPRINTS) ? "\033[32m✔\033[0m" : " ");
    printf(" - [%s] writing\n",  (mask & EV_WRITING)      ? "\033[32m✔\033[0m" : " ");
    printf(" - [%s] infrared\n", (mask & EV_INFRARED)     ? "\033[32m✔\033[0m" : " ");


    // Victory Results
    printf(
        "\n"
        "\033[35m"
        "=========================\n"
        "||   VICTORY RESULTS!    ||\n"
        "=========================\n"
        "\033[0m"
        "\n"
    );

    int exits_after_solve = 0;
    for (int i = 0; i < house.hunterCount; i++) {
        if (house.hunter[i].whyExit == LR_EVIDENCE)
            exits_after_solve++;
    }

    printf("- Hunters exited after identifying the ghost: %d/%d\n",
           exits_after_solve, house.hunterCount);

    printf("- Ghost Guess: N/A\n");
    printf("- Actual Ghost Type: %s\n", ghost_to_string(house.ghost.ghostType));

    // Final colored win/lose message
    if (exits_after_solve > 0) {
        printf("\nOverall Result: \033[32mHunters Win!\033[0m\n");
    } else {
        printf("\nOverall Result: \033[31mGhost Wins!\033[0m\n");
    }

    // Cleanup
    for (int i = 0; i < house.room_count; i++) {
        sem_destroy(&house.rooms[i].mutex);
    }
    sem_destroy(&house.fileCase.mutex);

    free(hunterThreads);
    free(house.hunter);

    return 0;
}
