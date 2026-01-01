# Multithreaded Ghost House Simulation (C)
Name: Gavin Joseph

## Overview
This project is a multithreaded simulation written in C that models a haunted house environment with concurrent agents. A ghost and multiple hunters operate simultaneously across shared rooms, interacting through shared state while leaving and collecting evidence. The simulation demonstrates systems-level programming concepts including concurrency, synchronization, shared memory coordination, and thread lifecycle management.
The project was implemented using POSIX threads (pthreads) and semaphores to ensure thread-safe execution and correct synchronization between agents.

## Key Concepts Demonstrated
- Multithreading with POSIX threads (pthreads)
- Synchronization using semaphores and mutexes
- Shared memory coordination
- Race condition and deadlock avoidance

## File Structure

- **main.c**
  - Handles program initialization and user input, populates the house with rooms, creates and manages ghost and hunter threads, and joins all threads at the end of the simulation. Prints final results including the case-file checklist and win condition, then releases allocated memory and destroys synchronization primitives.

- **functions.c**
  - Contains the core simulation logic, including initialization of ghost and hunter structures, state updates, evidence handling, and movement behavior. Implements stack-based path tracking for hunters and defines the behavior executed by hunter and ghost threads.

- **helpers.c**
  - Provides logging utilities to track ghost and hunter movements, along with a thread-safe random number generator (`rand_int_threadsafe`) and helper functions for populating rooms.

- **defs.h**
  - Defines shared data structures, enums, constants, and function prototypes used across the project.

- **helpers.h**
  - Contains function prototypes for helper and logging utilities.

- **Makefile**
  - Builds the project using `make` and removes generated logs and binaries using `make clean`.

- **validate_logs.py**
  - Verifies that generated CSV log files conform to the required format.

## How to Run the Project
### Prerequisites
- A Unix-based operating system (Linux or macOS)
- GCC or Clang with POSIX threads (pthreads) support
- `make`

### Steps
1. Clone the repository: git clone https://github.com/GavinJoseph27/ghost-house-simulation.git
2. Navigate into the project directory: cd ghost-house-simulation
3. Compile the project: make
4. Run the simulation: ./ghosthouse
5. To remove compiled files and generated logs: make clean

