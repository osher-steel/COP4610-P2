#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/list.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group #");
MODULE_DESCRIPTION("Timer Module");
MODULE_VERSION("1.0");

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL

#define NUM_FLOORS 6
#define MAX_LOAD 700
#define MAX_PASSENGERS 5
#define PART_TIME 0 
#define LAWYER 1
#define BOSS 2
#define VISITOR 3


int start_elevator(void);                                                          
int issue_request(int start_floor, int destination_floor, int type);               
int stop_elevator(void); 

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int,int,int);
extern int (*STUB_stop_elevator)(void);

enum state {OFFLINE, IDLE, LOADING, UP, DOWN};

struct Elevator{
    enum state;
    int current_load, current_floor, current_destination;
    struct list_head passengers_on_board;
};

typedef struct passenger{
    int destination, weight, start;
    struct list_head list;
} Passenger;

struct Floor{
    int num_waiting_floor;
    struct list_head passengers_waiting;
}

static struct proc_dir_entry* elevator_entry;

struct Elevator elevator;
struct Floor floors[6];

static int num_passengers;
static int num_waiting;
static int num_serviced;

static bool turn_off;

int start_elevator(void){
    if(elevator.state != OFFLINE){
        return 1;
    }

    elevator.current_floor = 1;
    elevator.current_load = 0;
    elevator.state = IDLE;

    num_passengers = 0;
    num_serviced = 0;
    num_waiting = 0;

    return 0;
    // add -ERRORNUM and -ENOMEM
}

int issue_request(int start_floor, int destination_floor, int type){
    if(start_floor < 1 || start_floor > 6 || destination_floor < 1 || destination_floor > 6 )
        return 1;

    int weight;

    switch(type){
        case PART_TIME:
            weight = 10;
            break;
        case LAWYER:
            weight = 15;
            break;
        case BOSS:
            weight = 20;
            break;
        case VISITOR:
            weight = 5;
            break;
        case default:
            return 1;
    }


    // Initialize passenger object
    struct Passenger *passenger = kmalloc(sizeof(struct Passenger), GFP_KERNEL);
    passenger.start = start_floor + 1;
    passenger.destination = destination_floor - 1;
    passenger.weight = weight;

    // Add passenger to floor list
    list_add_tail(&passenger->list, &floors[start_floor - 1].passengers_waiting);

    num_waiting ++;
}      

int stop_elevator(void){
    if(turn_off == true)
        return 1;
    
    turn_off = true;
    return 0;
}

// void run_elevator(){
//     switch(elevator.state){
//         case OFFLINE:
//             return;
//         case IDLE:
//             if(passengers_waiting >0){
//                 moveElevator();
//             }
//     }
// }

void moveElevator(void){
    if(elevator.current_floor == elevator.current_destination){
        if(num_waiting > 0)
            getNewDestination();
        else
            State = IDLE;
    }
    else if(elevator.current_floor < elevator.current_destination){
        // WAIT 2 SECs
        elevator.state = UP;
        elevator.current_floor += 1;
    }
    else if(elevator.current_floor > elevator.current_destination){
        // WAIT 2 SECs
        elevator.state = DOWN
        elevator.current_floor -= 1;
    }
}

void getNewDestination(void){
    // Loop through floors starting at the current floor and going up
    for(int i=0; i< NUM_FLOORS; i++){
        // If the floor has passengers waiting make it new destination floor
        if(!list_empty(&floors[(i+elevator.current_floor) % NUM_FLOORS].passengers_waiting)){
            elevator.current_destination = (i+elevator.current_floor) % NUM_FLOORS;
            return;
        }
    }
}

void service_floor(void){
	struct list_head *temp;
	struct list_head *dummy;
	Passenger *p;

    // Check if any passenger on board is at destination
    if(!list_empty(&elevator.passengers_on_board)){
        list_for_each_safe(temp, dummy, &elevator.passengers_on_board){
            p = list_entry(temp, Passenger, list);
	
            if(p.destination == elevator.current_floor){
                // WAIT 1 SEC

                elevator.state = LOADING;

                num_passengers--;
                num_serviced++;
                elevator.current_load -= p.weight;

                list_del(temp);	
                kfree(p);
            }
        }

        
    }

    // Check if there are any passengers waiting to board at current floor
    if(!turn_off && !list_empty(&floors[elevator.current_floor])){
        list_for_each_safe(temp, dummy, &floors[elevator.current_floor]){
            p = list_entry(temp, Passenger, list);

            if(num_passengers < 5 && elevator.current_load + p.weight <= MAX_LOAD){
                // WAIT 1 SEC
                elevator.state = LOADING;

                num_waiting --;
                num_passengers ++;
                elevator.current_load += passenger.weight
                
                // Add passenger to elevator list
                list_add_tail(&p->list, &elevator);

                // Remove passenger from floor list
                list_del(temp);	
                kfree(p);
            }
        }
    }
}


static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos){
    char buf[10000];
    int len = 0;

    len = sprintf(buf, "Elevator state: \n");
    len += sprintf(buf + len, "Current floor: \n");
    len += sprintf(buf + len, "Current load: \n");
    len += sprintf(buf + len, "Elevator status: \n");
    // Finish the rest.

    return simple_read_from_buffer(ubuf, count, ppos, buf, len);
}

static const struct proc_ops elevator_fops = {
    .proc_read = elevator_read,
};

static int __init elevator_init(void){
    STUB_start_elevator = start_elevator;
    STUB_issue_request = issue_request;
    STUB_stop_elevator = stop_elevator;
    

    elevator_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &elevator_fops);
    if (!elevator_entry) {
        return -ENOMEM;
    }

    state = OFFLINE;

    elevator.state = OFFLINE;
    INIT_LIST_HEADS(&elevator.passengers_on_board);

    for(int i=0; i<NUM_FLOORS; i++){
        floors[i].num_waiting_floor = 0;
        INIT_LIST_HEADS(&floors[i].passengers_waiting);
    }

    return 0;
}

static void __exit elevator_exit(void){
    // Free all passengers still waiting in each floor
    proc_remove(proc_entry);
}

module_init(elevator_init);
module_exit(elevator_exit);