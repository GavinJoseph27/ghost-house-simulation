Name: Gavin Joseph
Student Number: 101348882

Instructions for compiling and executing the code: To complie the code, you first set the directory to final using cd final, then you complie by writing make in the termial. After you do that, you do ./ghosthouse to run the file. To clean all the log files, use make clean. You can also complie it using gcc -Wall -Wextra -pthread -o ghosthouse main.c functions.c helpers.c. And then use ./ghosthouse to run it.

What is the purpose of each file: The purpose of main.c is that it handles user input, 
it is used to populate the house with rooms provided, it is used to create the ghost 
and hunter threads. It joins all threads after the simulation ends. It prints the final 
results, the case-file checklist, and whelter the hunter or the ghost wins. It releases 
all allocated memory and destroys semaphores. The purpose of functions.c is that it 
contains all the logic of the game. Like ghost_init, which initializes ghost state and 
starting room. Also, hunter_add, which creates and initializes hunter structures. Then, 
there is roomstack_push, roomstack_pop, roomstack_clear, which are stacks, that help 
the hunter keep track of the path they're going. Then, their is hunter_thread which 
keeps track of everything the hunter does using multithread. Finally, there is ghost_
thread which is a multithread that tracks the ghost movement and the evidence they 
leave. The purpose of helpers.c is for the logging functions, which keeps track of 
the movements that both the hunter and the ghost make. It also helps with the random 
integer generator function, rand_int_threadsafe. Then, there is the house_populate_rooms 
which helps populate all the rooms. The purpose of defs.h is to define all the structs, 
enums and constants. It also contains the function prototypes of functions.c. The purpose 
of helpers.h is to contain the funtion prototypes of helpers.c. Makefile builds the project, 
and it can clean up the logs using make clean and complie the code with make. The purpose 
of validate_logs.py is that it checks that all the log CSV files follow the required format. 

What bonus items have you included: Colored terminal output. 

What sources did you use to assist with coding and planning, such as course note snippets, lecture
slides, discussions with a peer: I used lecture slides 8, 10, 11, 12 from Section A of the COMP 2401 brightspace.

Any assumptions that you made as part of your implementation: Ghost may leave evidence randomly at around a 15% chance unless otherwise specified