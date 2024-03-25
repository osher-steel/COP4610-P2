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

#define OFFLINE OFFLINE
#define IDLE IDLE
#define LOADING LOADING
#define UP UP
#define DOWN DOWN

int start_elevator(void);                                                          
int issue_request(int start_floor, int destination_floor, int type);               
int stop_elevator(void); 
void moveElevator(void);

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int,int,int);
extern int (*STUB_stop_elevator)(void);

enum Elevator_state {OFFLINE, IDLE, LOADING, UP, DOWN};

struct Elevator{
    enum Elevator_state state;
    int current_load, current_floor, current_destination;
    struct list_head passengers_on_board;
    struct task_struct *kthread;
    struct mutex mutex;
};

typedef struct passenger{
    int destination, weight, start;
    struct list_head list;
    char str[2];
} Passenger;

struct Floor{
    int num_waiting_floor;
    struct list_head passengers_waiting;
};

void getNewDestination(void);
void service_floor(void);
static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos);

static struct proc_dir_entry* elevator_entry;

struct Elevator elevator;
struct Floor floors[NUM_FLOORS];

static int num_passengers;
static int num_waiting;
static int num_serviced;

static bool turn_off;

int start_elevator(void){
    //mutex_lock(&elevator.mutex);
    if(elevator.state != OFFLINE){
        //mutex_lock(&elevator.mutex);
        return 1;
    }

    elevator.current_floor = 1;
    elevator.current_load = 0;
    elevator.state = IDLE;

    turn_off = false;
    //mutex_unlock(&elevator.mutex);
    return 0;
    // add -ERRORNUM and -ENOMEM
}

int issue_request(int start_floor, int destination_floor, int type){
    printk(KERN_INFO "Inside Issue request");
    //mutex_lock(&elevator.mutex);
    /*if(turn_off || elevator.state == OFFLINE || start_floor < 1 || start_floor > NUM_FLOORS || destination_floor < 1 || destination_floor > NUM_FLOORS )
        //mutex_unlock(&elevator.mutex);
        printk(KERN_INFO "returning early");
        return 1;
        */
    

    int weight;
    char initial;

    switch(type){
        case PART_TIME:
            weight = 10;
            initial = 'P';
            break;
        case LAWYER:
            weight = 15;
            initial = 'L';
            break;
        case BOSS:
            weight = 20;
            initial = 'B';
            break;
        case VISITOR:
            weight = 5;
            initial = 'V';
            break;
        default:
            return 1;
    }

    // Initialize passenger
    printk(KERN_INFO "test");
    Passenger *passenger;
    printk(KERN_INFO "test1");
    passenger = kmalloc(sizeof(Passenger), GFP_KERNEL);
    printk(KERN_INFO "test2");
    passenger->start = start_floor + 1;
    passenger->destination = destination_floor - 1;
    passenger->weight = weight;
    printk(KERN_INFO "test3");

    sprintf(passenger->str, "%c%d", initial, destination_floor);
    printk(KERN_INFO "test4");
    
    // Add passenger to floor list
    printk(KERN_INFO "test5");
    list_add_tail(&passenger->list, &floors[start_floor - 1].passengers_waiting);
    printk(KERN_INFO "adding passengers");
    num_waiting++;

    //mutex_unlock(&elevator.mutex);
    return 0;
}      

int stop_elevator(void){
    //mutex_lock(&elevator.mutex);
    if(elevator.state == OFFLINE || turn_off)
        //mutex_unlock(&elevator.mutex);
        return 1;
    
    turn_off = true;
    //mutex_unlock(&elevator.mutex);
    return 0;
}

int elevator_run(void *data){
    while(!kthread_should_stop()){
        printk(KERN_INFO "kthread");
        mutex_lock(&elevator.mutex);
        if(elevator.state != OFFLINE){
            printk(KERN_INFO "online");
            if(num_waiting > 0){
                printk(KERN_INFO "people waiting");
                if(elevator.state == IDLE){
                    printk(KERN_INFO "Getting new destination");
                    getNewDestination();
                    printk(KERN_INFO "got new destination");
                }

                service_floor();
                moveElevator();
                printk(KERN_INFO "floor and elevator moved");
            }
            else{
                if(turn_off){
                
                    printk(KERN_INFO "offline");
                    elevator.state = OFFLINE;
                    }
                else{
                    printk(KERN_INFO "idle");
                    elevator.state = IDLE;
                }
            }
        }
        //mutex_unlock(&elevator.mutex);
    }
    return 0;
}

void moveElevator(void){
    //mutex_lock(&elevator.mutex);
    printk(KERN_INFO "moving the elevator");
    if(elevator.current_floor == elevator.current_destination){
        if(num_waiting > 0)
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
    //mutex_unlock(&elevator.mutex);
}

void getNewDestination(void){
    // Loop through floors starting at the current floor and going up
    //mutex_lock(&elevator.mutex);
    printk(KERN_INFO "getting new destination");
    for(int i=0; i< NUM_FLOORS; i++){
        // If the floor has passengers waiting make it new destination floor
        if(!list_empty(&floors[(i+elevator.current_floor) % NUM_FLOORS].passengers_waiting)){
            elevator.current_destination = (i+elevator.current_floor) % NUM_FLOORS;
            printk(KERN_INFO "new destination");
            //mutex_unlock(&elevator.mutex);
            return;
        }

    }
    printk(KERN_INFO "no destination found");
    return;

    //mutex_unlock(&elevator.mutex);
}

void service_floor(void){
    //mutex_lock(&elevator.mutex);
	struct list_head *temp;
	struct list_head *dummy;
	Passenger *p;

    // Check if any passenger on board is at destination
    if(!list_empty(&elevator.passengers_on_board)){
        list_for_each_safe(temp, dummy, &elevator.passengers_on_board){
            p = list_entry(temp, Passenger, list);
	
            if(p->destination == elevator.current_floor){
                elevator.state = LOADING;
                ssleep(1);

                num_passengers--;
                num_serviced++;
                elevator.current_load -= p->weight;

                list_del(temp);
                kfree(p);
            }
        }

        
    }

    // Check if there are any passengers waiting to board at current floor
    if(!turn_off && !list_empty(&floors[elevator.current_floor].passengers_waiting)){
        list_for_each_safe(temp, dummy, &floors[elevator.current_floor].passengers_waiting){
            p = list_entry(temp, Passenger, list);

            if(num_passengers < 5 && elevator.current_load + p->weight <= MAX_LOAD){
                elevator.state = LOADING;
                ssleep(1);

                num_waiting--;
                num_passengers++;
                elevator.current_load += p->weight;
                
                // Add passenger to elevator list
                list_add_tail(&p->list, &elevator.passengers_on_board);

                // Remove passenger from floor list
                list_del(temp);	
                kfree(p);
            }
        }
    }
    //mutex_unlock(&elevator.mutex);
}


static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos){
    char buf[10000];
    int len = 0;
    const char *state="";
    switch(elevator.state){
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
    

    len = sprintf(buf, "Elevator state:");
    len += sprintf(buf + len, state);
    len += sprintf(buf + len, "\nCurrent floor: ");
    len += sprintf(buf + len, "%d", elevator.current_floor);
    len += sprintf(buf + len, "\nCurrent load: ");
    len += sprintf(buf + len, "%d", elevator.current_load);
    len += sprintf(buf + len, "\nElevator status: ");

    if(!list_empty(&elevator.passengers_on_board)){
        struct list_head *temp;
        Passenger *passenger;

        list_for_each(temp,&elevator.passengers_on_board){
            passenger = list_entry(temp, Passenger,list);
            len += sprintf(buf + len, passenger->str);
        }
    }

    for(int i=0; i<NUM_FLOORS; i++){
        len += sprintf(buf + len, "\n");
        len += sprintf(buf + len, "[");

         if(i == elevator.current_floor-1)
            len += sprintf(buf + len, "*]");
        else
            len += sprintf(buf + len, " ]");
        
        
        len += sprintf(buf + len, " Floor ");
        len += sprintf(buf + len, "%d", i+1);

        len += sprintf(buf + len, ": ");

        if(!list_empty(&floors[i].passengers_waiting)){
            struct list_head *temp;
            Passenger *passenger;

            list_for_each(temp,&floors[i].passengers_waiting){
                passenger = list_entry(temp, Passenger,list);
                len += sprintf(buf + len, passenger->str);
            }
        }
    }

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

    elevator.state = OFFLINE;
    INIT_LIST_HEAD(&elevator.passengers_on_board);

    mutex_init(&elevator.mutex);

    // Needs to be modified
    elevator.kthread = kthread_run(elevator_run, &elevator, "elevator thread");

    for(int i=0; i<NUM_FLOORS; i++){
        floors[i].num_waiting_floor = 0;
        INIT_LIST_HEAD(&floors[i].passengers_waiting);
    }


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

    if(num_passengers > 0){
        list_for_each_safe(temp, dummy, &elevator.passengers_on_board){
            p = list_entry(temp, Passenger, list);

            list_del(temp);	
            kfree(p);
        }
    }

    if(num_waiting > 0){
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

    remove_proc_entry(ENTRY_NAME, NULL);
    mutex_destroy(&elevator.mutex);
}

module_init(elevator_init);
module_exit(elevator_exit);
