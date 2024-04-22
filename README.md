
# Project 2

Overview of assignment 
```
1. System Call Tracing
2. Timer Kernel Module
3. Elevator Module
```

## Group Members

- **Jack Throdal**
- **Trent Fetzer**
- **Osher Steel**
 
## Division of Labor

### Part 1: System Call Tracing
Assigned to: Osher Steel, Trent Fetzer, Jack Throdal
### Part 2: Timer Kernel Module
Assigned to: Osher Steel, Trent Fetzer, Jack Throdal
### Part 3a: Adding System Calls
  Assigned to: Trent Fetzer, Jack Throdal
### Part 3b: Kernel Compilation
  Assigned to: Osher Steel, Jack Throdal
### Part 3c: Threads
  Assigned to: Osher Steel, Trent Fetzer
### Part 3d: Linked Lists
  Assigned to: Trent Fetzer, Jack Throdal
### Part 3e: Mutexes
  Assigned to: Osher Steel, Jack Throdal
### Part 3f: Scheduling Algorithms
  Assigned to: Osher Steel, Trent Fetzer

## File Listing
```
├── Part1
  └── empty.c
  └── part1.c
├── Part2
  └── Makefile
  └── my_timer.c
├── Part3
  └── elevator
    └── Makefile
    └── elevator.c
    └── wrappers.c
    └── wrappers.h
  
├── readme.md
└── src
  └── filesys.c
```

## How to Compile & Execute

### Requirements
- **Compiler**: `gcc` for C/C++

### Compilation
Part 1: Inside Part1 run `make` to create empty and part1 object files

Part 2: Inside Part2 run `make` to create the kernel object and then `sudo insmod my_timer.ko` to insert the kernel module

Part 3: Inside Part3/elevator run `make` to create the kernel object and then `sudo insmod elevator.ko` to insert the kernel module
        Inside Part3/elevator_testing run `make` to create producer and consumer object files

### Execution
Part 1: Observe the system calls when executing the object files using `strace -o [file].trace ./[object]`
Part 2: Look at the procfile using `cat /proc/timer`
Part 3: Start the elevator with `./consumer --start` and stop it with `./consumer --stop` 
        Add passengers to the elevator with `./producer [number_of_passengers]`

## Bugs
