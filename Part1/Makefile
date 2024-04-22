CC = gcc
CFLAGS = -Wall -O2  # Compiler flags

# Target: all (default target)
all: empty part1

# Target: empty (compile empty.c)
empty: empty.c
	$(CC) $(CFLAGS) -o empty empty.c

# Target: part1 (compile part1.c)
part1: part1.c
	$(CC) $(CFLAGS) -o part1 part1.c

# Target: clean (clean up built files)
clean:
	rm -f empty part1

# Phony target: clean (specify it's not a file)
.PHONY: all clean
