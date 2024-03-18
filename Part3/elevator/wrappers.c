#include "wrappers.h"

int start_elevator() {
    /*
    State = IDLE
    Current Floor = 1
    Current Load = 0
    */
    return syscall(__NR_START_ELEVATOR);
}

int issue_request(int start, int dest, int type) {
    /*
        Passenger p(start,dest,type);
        floor[start].add(p);

        current_waiting ++;
    */
    return syscall(__NR_ISSUE_REQUEST, start, dest, type);
}

int stop_elevator() {
    /*
        Stop = true
    */
    return syscall(__NR_STOP_ELEVATOR);
}
