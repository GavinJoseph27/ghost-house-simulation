# Compiler to use
CC = gcc 

# Compilation flags: enable warnings and pthread support
CFLAGS = -Wall -Wextra -pthread 

# Object files required to build the program
OBJS = main.o functions.o helpers.o 

# Default target: build the ghosthouse executable
all: ghosthouse

# Link all object files into the final executable
ghosthouse: $(OBJS)
	$(CC) $(CFLAGS) -o ghosthouse $(OBJS)

# Compile main.c into main.o
main.o: main.c defs.h helpers.h
	$(CC) $(CFLAGS) -c main.c

# Compile functions.c into functions.o
functions.o: functions.c defs.h helpers.h
	$(CC) $(CFLAGS) -c functions.c

# Compile helpers.c into helpers.o
helpers.o: helpers.c defs.h helpers.h
	$(CC) $(CFLAGS) -c helpers.c

# Clean all object files, executable, and generated log files
clean:
	rm -f *.o ghosthouse log_*.csv