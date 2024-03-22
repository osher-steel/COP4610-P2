#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group #");
MODULE_DESCRIPTION("Timer Module");
MODULE_VERSION("1.0");

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL

int start_elevator(void);                                                          
int issue_request(int start_floor, int destination_floor, int type);               
int stop_elevator(void); 

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int,int,int);
extern int (*STUB_stop_elevator)(void);

enum state {OFFLINE, IDLE, LOADING, UP, DOWN};

struct Passenger{
    int destination, weight, start;
};


enum state;
int current_floor;
int current_load;
int current_destination;

int num_passengers;
int num_waiting;
int num_serviced;

bool active;
bool turn_off;


// List [6] Floor  (Array of 6 floors which each contains a list of passengers waiting)
// List Elevator (List that contains passengers in elevator)

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int,int,int);
extern int (*STUB_stop_elevator)(void);

int start_elevator(void){
    if(state == OFFLINE){
        return 1;
    }

    current_floor = 1;
    current_load = 0;
    state = IDLE
    return 0;

    // add -ERRORNUM and -ENOMEM
}

int issue_request(int start_floor, int destination_floor, int type){
    int weight;

    switch(type){
        case 0:
            weight = 10;
            break;
        case 1:
            weight = 15;
            break;
        case 2:
            weight = 20;
            break;
        case 3:
            weight = 5;
            break;
    }

    Passenger passenger = Passenger(start,dest,weight);
    // floor[start].add(p);

    current_waiting ++;
};        

int stop_elevator(void){
    turn_off = true;
}

void moveElevator(void){
    if(current_destination == 0){
        getNewDestination();
    }

    if(current_floor < current_destination){
        goUp()
    }
    else if(current_floor > current_destination){
        doDown()
    }
    
    check_floor()

    if(current_floor == current_destination){
        if(num_waiting > 0){
            getNewDestination()
        }
        else
            State = IDLE
    }


};


void getNewDestination(void){
    // Loop through floors:
        // if floor is not empty:
            current_destination = floor_num;
                return;

    state = IDLE;
}


void goUp(void){
    state = UP
    current_floor += 1
};

void goDown(void){
    state = DOWN
    current_floor -= 1
};

void check_floor(void){
    // loop through elevator list:
        if(passenger.destination == current_floor)
            dropOff(passenger);
    
    // loop through floor[current_floor] list:
        if(!stop && current_load + passenger.weight <= MAX_LOAD)
            load(passenger);
};

void dropOff(Passenger passenger){
    State = LOADING;
    num_passengers--;
    num_serviced++;
    current_load -= passenger.weight;
    // delete passenger from elevator list
}

void load(Passenger passenger){
    STATE = LOADING;
    num_waiting --;
    num_passengers ++;
    current_load += passenger.weight
    // delete passenger from floor list
    // add passenger to elevator list
}

// write_to_proc(){
//     generate text and then write to proc

//     char star[6];
//     for(i = 0; i<5; i++)
//         if(i == current_floor)
//             star[i] = '*'
//         else
//             star[i] = ' '
    
//     string f[6]
//     string waiting[6]

//     loop through each floor:
//         sum = 0
//         loop through each item in list:
//             f[i].append(passenger)
//             sum ++
        
//         waiting[i] = sum

//     text = f" 
//     Elevator State :{state} \n
//     Current Floor : {current_floor} \n
//     Current Load : {load}

//     [{star[5]}] Floor 6 : {waiting[5]} {f[5]} 
//     [{star[4]}] Floor 5 : {waiting[4]} {f[4]} 
//     [{star[3]}] Floor 4 : {waiting[3]} {f[3]} 
//     [{star[2]}] Floor 3 : {waiting[2]} {f[2]} 
//     [{star[1]}] Floor 2 : {waiting[1]} {f[1]} 
//     [{star[0]}] Floor 1 : {waiting[0]} {f[0]} 

//     Number of passengers : {num_passengers}
//     Number of passengers waiting : {num_waiting}
//     Number of passengers serviced : {num_serviced}"
    
//     write text to procfile

// }

static int __init elevator_init(void){
    STUB_start_elevator = start_elevator;
    STUB_issue_request = issue_request;
    STUB_stop_elevator = stop_elevator;
    
    proc_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &procfile_fops);

    state = OFFLINE;

};

static void __exit elevator_exit(void){
        proc_remove(proc_entry);
};

module_init(elevator_init);
module_exit(elevator_exit);