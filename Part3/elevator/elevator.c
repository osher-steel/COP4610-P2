#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group #");
MODULE_DESCRIPTION("Elevator Module");
MODULE_VERSION("1.0");

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL

#define NUM_FLOORS 5
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
    struct task_struct *kthread;
    struct mutex mutex;
};

typedef struct passenger{
    int destination, weight, start;
    struct list_head list;
} Passenger;

struct Floor{
    int num_waiting_floor;
    struct list_head passengers_waiting;
};

static struct proc_dir_entry* elevator_entry;

struct Elevator elevator;
struct Floor floors[NUM_FLOORS];

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

    turn_off = false;

    return 0;
    // add -ERRORNUM and -ENOMEM
}

int issue_request(int start_floor, int destination_floor, int type){
    if(turn_off || elevator.state == OFFLINE || start_floor < 1 || start_floor > NUM_FLOORS || destination_floor < 1 || destination_floor > NUM_FLOORS )
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

    // Initialize passenger
    struct Passenger *passenger = kmalloc(sizeof(struct Passenger), GFP_KERNEL);
    passenger.start = start_floor + 1;
    passenger.destination = destination_floor - 1;
    passenger.weight = weight;

    // Add passenger to floor list
    list_add_tail(&passenger->list, &floors[start_floor - 1].passengers_waiting);

    num_waiting ++;

    return 0;
}      

int stop_elevator(void){
    if(elevator.state == OFFLINE || turn_off)
        return 1;
    
    turn_off = true;
    return 0;
}


void elevator_run(void){
    while(!kthread_should_stop()){
        if(elevator.state != OFFLINE){
            if(passengers_waiting > 0){
                if(elevator.state == IDLE)
                    getNewDestination();
                
                service_floor();
                moveElevator();
            }
            else{
                if(turn_off)
                    elevator.state = OFFLINE;
                else
                    elevator.state = IDLE;
            }
        }
    }
}

void moveElevator(void){
    if(elevator.current_floor == elevator.current_destination){
        if(passengers_waiting > 0)
            getNewDestination();
        else
            elevator.state = IDLE;
    }
    else if(elevator.current_floor < elevator.current_destination){
        elevator.state = UP;
        ssleep(2);
        elevator.current_floor += 1;
    }
    else if(elevator.current_floor > elevator.current_destination){
        elevator.state = DOWN;
        ssleep(2);
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
                elevator.state = LOADING;
                ssleep(1);

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
                elevator.state = LOADING;
                ssleep(1);

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

    elevator.state = OFFLINE;
    INIT_LIST_HEADS(&elevator.passengers_on_board);

    mutex_init(&elevator.mutex);

    // Needs to be modified
    elevator->thread = kthread_run(elevator_run, parm, "elevator thread", parm->id);

    for(int i=0; i<NUM_FLOORS; i++){
        floors[i].num_waiting_floor = 0;
        INIT_LIST_HEADS(&floors[i].passengers_waiting);
    }


    num_passengers = 0;
    num_serviced = 0;
    num_waiting = 0;

    return 0;
}

static void __exit elevator_exit(void){
    kthread_stop(elevator->kthread);

    struct list_head *temp;
	struct list_head *dummy;
	Passenger *p;

    if(num_passengers > 0){
        list_for_each_safe(temp, dummy, &elevator.passengers_on_board){
            p = list_entry(temp, Passenger, list);

            list_del(temp);	
            kfree(p);
        }
    }

    if(num_waiting > 0){
        for(int i=0; i< NUM_FLOORS; i++){
            if(!list_empty(&floors[elevator.current_floor])){
                list_for_each_safe(temp, dummy, &floors[elevator.current_floor]){
                    p = list_entry(temp, Passenger, list);

                    list_del(temp);	
                    kfree(p);
                }
            }
        }
    }

    proc_remove(proc_entry);
    mutex_destroy(&elevator.mutex);
}

module_init(elevator_init);
module_exit(elevator_exit);