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
MODULE_AUTHOR("Group #21");
MODULE_DESCRIPTION("Elevator Module");
MODULE_VERSION("1.0");

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL

//elevator specifications and passenger weights
#define NUM_FLOORS 5
#define MAX_LOAD 70
#define MAX_PASSENGERS 5
#define PART_TIME 0 
#define LAWYER 1
#define BOSS 2
#define VISITOR 3

//Elevator status
#define OFFLINE OFFLINE
#define IDLE IDLE
#define LOADING LOADING
#define UP UP
#define DOWN DOWN

//function definitions
static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos);
int start_elevator(void);                                                          
int issue_request(int start_floor, int destination_floor, int type);               
int stop_elevator(void); 
void moveElevator(void);
void getNewDestination(void);
void service_floor(void);

//system calls
extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int,int,int);
extern int (*STUB_stop_elevator)(void);

//elevator states
enum Elevator_state {OFFLINE, IDLE, LOADING, UP, DOWN};

//elevator struct
struct Elevator{
    enum Elevator_state state;//current thread
    int current_load, current_floor, current_destination;
    struct list_head passengers_on_board;//passengers loaded from floors
    struct task_struct *kthread;
    struct mutex mutex;//locks
};

//passenger specifictions
typedef struct passenger{
    int destination, weight, start;//via issue request
    struct list_head list;
    char str[2];//passenger type/weight
} Passenger;

//floor to hold waiting passengers
struct Floor{
    int num_waiting_floor;
    struct list_head passengers_waiting;
};

static struct proc_dir_entry* elevator_entry;

//create elevator and floors
struct Elevator elevator;
struct Floor floors[NUM_FLOORS];


static int num_passengers;
static int num_waiting;
static int num_serviced;

//to turn elevatot offline
static bool turn_off;

int start_elevator(void){
    //start lock
    mutex_lock(&elevator.mutex);
    if(elevator.state != OFFLINE)
    {
        mutex_lock(&elevator.mutex);
        return 1;
    }
    if(elevator.current_floor > 0) {}//if elevator is not at 1, keep it at its location
    else
        elevator.current_floor = 1;
    //prepare for passengers
    elevator.current_load = 0;
    elevator.state = IDLE;

    turn_off = false;
    mutex_unlock(&elevator.mutex);
    return 0;
    
}

int issue_request(int start_floor, int destination_floor, int type){
    mutex_lock(&elevator.mutex);
    if(turn_off || elevator.state == OFFLINE || start_floor < 1 || start_floor > NUM_FLOORS || destination_floor < 1 || destination_floor > NUM_FLOORS )
    {
        mutex_unlock(&elevator.mutex);
        return 1;
    }
    
    int weight;//passenger weight and type
    char initial;

    switch(type){
        case PART_TIME:
            weight = 10;
            initial = 'P';//Part Timer
            break;
        case LAWYER:
            weight = 15;
            initial = 'L';//Lawyer 1.5
            break;
        case BOSS:
            weight = 20;
            initial = 'B';//Boss 2.0
            break;
        case VISITOR:
            weight = 5;
            initial = 'V';//Visitor .5
            break;
        default:
            return 1;
    }

    //initalize new passenger
    Passenger *passenger;
    passenger = kmalloc(sizeof(Passenger), GFP_KERNEL);
    //set variables
    passenger->start = start_floor - 1;
    passenger->destination = destination_floor - 1;
    passenger->weight = weight;
    
    sprintf(passenger->str, "%c%d", initial, destination_floor);
    //add passengers to list of passengers waiting
    list_add_tail(&passenger->list, &floors[start_floor - 1].passengers_waiting);
    num_waiting++;//new passenger weighting

    mutex_unlock(&elevator.mutex);
    return 0;
}      

//call to turn elevator offline
int stop_elevator(void){
    mutex_lock(&elevator.mutex);
    if(elevator.state == OFFLINE || turn_off)
    {
        mutex_unlock(&elevator.mutex);
        return 1;
    }
    
    turn_off = true;
    mutex_unlock(&elevator.mutex);
    return 0;
}

int elevator_run(void *data) {
    while (!kthread_should_stop()) {
        //if call to turn offline and empty elevator, turn offline
        if (turn_off && list_empty(&elevator.passengers_on_board) && elevator.state != OFFLINE) {
            elevator.state = OFFLINE;
        }
        //else run elevator
        mutex_lock(&elevator.mutex);
        switch (elevator.state) {
            case IDLE://wait for passengers
                if (!list_empty(&floors[elevator.current_floor - 1].passengers_waiting) || !list_empty(&elevator.passengers_on_board)) {
                    elevator.state = LOADING;
                } 
                else 
                {
                    for (int i = 0; i < NUM_FLOORS; ++i) {//loop thrugh floors
                        if (!list_empty(&floors[i].passengers_waiting)) {
                            elevator.current_destination = i + 1;
                            elevator.state = (i + 1 > elevator.current_floor) ? UP : DOWN;
                            break;
                        }
                    }
                }
                break;
            case LOADING://add passengers to elevator
                if (!list_empty(&floors[elevator.current_floor - 1].passengers_waiting)) {
                    struct list_head *temp;
                    struct list_head *dummy;
                    //loop thru passengers waiting
                    list_for_each_safe(temp, dummy, &floors[elevator.current_floor - 1].passengers_waiting) {
                        Passenger *passenger = list_entry(temp, Passenger, list);
                        //if new passenger doesnt exceed max load
                        if (elevator.current_load + passenger->weight <= MAX_LOAD) {
                            list_move_tail(temp, &elevator.passengers_on_board);
                            elevator.current_load += passenger->weight;//add cur pass weight
                            num_waiting--;//remove weighter
                            num_passengers++;//add elev pass
                            if (num_passengers == MAX_PASSENGERS) break;//cant accept anymore, break from loop
                        }
                    }
                }
                //get new destination
                if (!list_empty(&elevator.passengers_on_board)) {
                    getNewDestination();
                    elevator.state = (elevator.current_destination > elevator.current_floor) ? UP : DOWN;
                } else {//else no passengers
                    elevator.state = IDLE;
                }
                ssleep(1);//sleep to load/unload passengers
                break;
            case UP:
            case DOWN:
                elevator.current_floor += (elevator.state == UP) ? 1 : -1;//move on case
                service_floor();//load/unload passengers
                if (elevator.current_floor == elevator.current_destination) {
                    if (!list_empty(&floors[elevator.current_floor - 1].passengers_waiting) || !list_empty(&elevator.passengers_on_board)) {
                        elevator.state = LOADING;//load passengers waiting
                    } else {
                        elevator.state = IDLE;//no passengers
                    }
                }
                ssleep(2);//wait while moving between floors
                break;
            default:
                break;
        }
        mutex_unlock(&elevator.mutex);
    }
    return 0;
}

void getNewDestination(void) {
    if (!list_empty(&elevator.passengers_on_board)) {
        Passenger *first_passenger;
        //get first elevator passengers destination and go
        first_passenger = list_first_entry(&elevator.passengers_on_board, Passenger, list);
        elevator.current_destination = first_passenger->destination + 1;
    } else {//else go to waiting passenger
        for (int i = 0; i < NUM_FLOORS; i++) {
            if (!list_empty(&floors[i].passengers_waiting)) {
                elevator.current_destination = i + 1;
                break;
            }
        }
    }
}

void service_floor(void) {
    struct list_head *temp, *dummy;
    list_for_each_safe(temp, dummy, &elevator.passengers_on_board) {//loop thru elevator passengers
        Passenger *passenger = list_entry(temp, Passenger, list);
        if (passenger->destination + 1 == elevator.current_floor) {//if pass at destination unload
            elevator.current_load -= passenger->weight;//loss weight
            list_del(temp);//free up space
            kfree(passenger);
            num_passengers--;//decrement variables
            num_serviced++;
        }
    }
    
    if (list_empty(&elevator.passengers_on_board)) {//loop thru passengers on board
        int found = 0;
        for (int i = 0; i < NUM_FLOORS; i++) {//loop through floors
            if (!list_empty(&floors[i].passengers_waiting)) {
                found = 1;//if passenger waiting, found
                break;
            }
        }
        if (!found) elevator.state = IDLE;//not found, idle
    }
}


static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos){
    char buf[10000];
    int len = 0;
    const char *state="";
    switch(elevator.state){//set elevator enum to char to write to proc
        case OFFLINE:
            state = "OFFLINE";
            break;
        case IDLE:
            state = "IDLE";
            break;
        case LOADING:
            state = "LOADING";
            break;
        case UP:
            state = "UP";
            break;
        case DOWN:
            state = "DOWN";
            break;
        default:
            state = "unknown";

    }
    
    len = sprintf(buf, "Elevator state: ");
    len += sprintf(buf + len, state);
    len += sprintf(buf + len, "\nCurrent floor: ");
    len += sprintf(buf + len, "%d", elevator.current_floor);
    len += sprintf(buf + len, "\nCurrent load: ");
    len += sprintf(buf + len, "%d", elevator.current_load);
    len += sprintf(buf + len, "\nElevator status: ");

    if(!list_empty(&elevator.passengers_on_board)){//print current passengers on board
        struct list_head *temp;
        Passenger *passenger;

        list_for_each(temp,&elevator.passengers_on_board){//list passenger and type, lawyer, boss, etc
            passenger = list_entry(temp, Passenger,list);
            len += sprintf(buf + len, passenger->str);
        }
    }

    for(int i=0; i<NUM_FLOORS; i++){
        len += sprintf(buf + len, "\n");
        len += sprintf(buf + len, "[");
        //to print which floor elevator is at
        if(i == elevator.current_floor-1)
        {len += sprintf(buf + len, "*]");}
        else
        {len += sprintf(buf + len, " ]");}
        
        len += sprintf(buf + len, " Floor ");
        len += sprintf(buf + len, "%d", i+1);

        len += sprintf(buf + len, ": ");

        if(!list_empty(&floors[i].passengers_waiting)){//print passengers waiting
            struct list_head *temp;
            Passenger *passenger;

            list_for_each(temp,&floors[i].passengers_waiting){
                passenger = list_entry(temp, Passenger,list);
                len += sprintf(buf + len, passenger->str);
            }
        }
    }
    //elevator stats
    len += sprintf(buf + len, "\nNumber of passengers: ");
    len += sprintf(buf + len, "%d", num_passengers);
    len += sprintf(buf + len, "\nNumber of passengers waiting: ");
    len += sprintf(buf + len, "%d", num_waiting);
    len += sprintf(buf + len, "\nNumber of passengers serviced :");
    len += sprintf(buf + len, "%d", num_serviced);

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

    elevator.state = OFFLINE;//elevator starts online
    INIT_LIST_HEAD(&elevator.passengers_on_board);

    mutex_init(&elevator.mutex);//initliaze ock

    elevator.kthread = kthread_run(elevator_run, &elevator, "elevator thread");

    for(int i=0; i<NUM_FLOORS; i++){//initalize empty floors
        floors[i].num_waiting_floor = 0;
        INIT_LIST_HEAD(&floors[i].passengers_waiting);
    }
    //empty stats to start
    num_passengers = 0;
    num_serviced = 0;
    num_waiting = 0;

    return 0;
}

static void __exit elevator_exit(void){
    kthread_stop(elevator.kthread);

    struct list_head *temp;
	struct list_head *dummy;
	Passenger *p;

    if(num_passengers > 0){//clear thread of elevator riders to remove elevators
        list_for_each_safe(temp, dummy, &elevator.passengers_on_board){
            p = list_entry(temp, Passenger, list);
            list_del(temp);	
            kfree(p);
        }
    }

    if(num_waiting > 0){//clear thread of passengers waiting to remove elevators
        for(int i=0; i< NUM_FLOORS; i++){
            if(!list_empty(&floors[elevator.current_floor].passengers_waiting)){
                list_for_each_safe(temp, dummy, &floors[elevator.current_floor].passengers_waiting){
                    p = list_entry(temp, Passenger, list);
                    list_del(temp);	
                    kfree(p);
                }
            }
        }
    }
    //destroy lock and proc 
    remove_proc_entry(ENTRY_NAME, NULL);
    mutex_destroy(&elevator.mutex);
}

module_init(elevator_init);
module_exit(elevator_exit);
