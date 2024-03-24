#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group 21");
MODULE_DESCRIPTION("Elevator Module");

#define MAX_WEIGHT 70 // 7 lbs, represented as integer
#define MAX_PASSENGERS 5
#define NUM_FLOORS 5

// Passenger types and their weights (multiplied by 10 for integer representation)
#define TYPE_PART_TIMER 10 // 1.0 lbs
#define TYPE_LAWYER 15     // 1.5 lbs
#define TYPE_BOSS 20       // 2.0 lbs
#define TYPE_VISITOR 5     // 0.5 lbs

struct passenger {
    int type;
    int start_floor;
    int destination_floor;
    struct list_head list;
};

struct elevator {
    struct mutex lock;
    struct task_struct *thread;
    struct list_head passengers;
    int current_floor;
    int direction; // 1 for up, -1 for down, 0 for idle
    int weight;
    int num_passengers;
};
static int elevator_proc_show(struct seq_file *m, void *v);
static struct seq_operations elevator_seq_ops = {
    .show = elevator_proc_show,
};
static int elevator_proc_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &elevator_seq_ops);
}

static const struct proc_ops elevator_proc_fops = 
{
    .proc_open = elevator_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static struct elevator elevator;

int calculate_weight(int type) {
    switch (type) {
        case 0: return TYPE_PART_TIMER;
        case 1: return TYPE_LAWYER;
        case 2: return TYPE_BOSS;
        case 3: return TYPE_VISITOR;
        default: return 0;
    }
}


// Elevator thread function
int elevator_function(void *data) {
    while (!kthread_should_stop()) {
        struct passenger *passenger;
        struct list_head *temp, *q;

        mutex_lock(&elevator.lock);

        if (!list_empty(&elevator.passengers)) {
            list_for_each_safe(temp, q, &elevator.passengers) {
                passenger = list_entry(temp, struct passenger, list);

                // Simulate moving to the passenger's start floor
                while (elevator.current_floor != passenger->start_floor) {
                    msleep(2000); // Simulate time to move between floors
                    elevator.current_floor += (passenger->start_floor > elevator.current_floor) ? 1 : -1;
                    printk(KERN_INFO "Elevator moved to floor %d\n", elevator.current_floor);
                }

                // Simulate loading passenger
                msleep(1000); // Simulate time to load passenger
                printk(KERN_INFO "Loading passenger at floor %d to floor %d\n", passenger->start_floor, passenger->destination_floor);
                
                // Simulate moving to the passenger's destination floor
                while (elevator.current_floor != passenger->destination_floor) {
                    msleep(2000); // Simulate time to move between floors
                    elevator.current_floor += (passenger->destination_floor > elevator.current_floor) ? 1 : -1;
                    printk(KERN_INFO "Elevator moved to floor %d\n", elevator.current_floor);
                }

                // Simulate unloading passenger
                msleep(1000); // Simulate time to unload passenger
                printk(KERN_INFO "Unloading passenger at floor %d\n", passenger->destination_floor);

                // Remove passenger from list
                list_del(temp);
                kfree(passenger);
                break; // Break after handling one passenger to recheck if the thread should stop
            }
        } else {
            // If there are no passengers, elevator stays idle for a moment before checking again
            msleep(1000); // Idle time
        }

        mutex_unlock(&elevator.lock);
        // Yield processor to prevent starvation of other processes/threads
        schedule();
    }
    return 0;
}


// Simulated system call to add a passenger
int issue_request(int start_floor, int destination_floor, int type) {
    struct passenger *new_passenger;
    int new_weight;
    new_weight = calculate_weight(type);
    if ((elevator.weight + new_weight > MAX_WEIGHT) || elevator.num_passengers >= MAX_PASSENGERS)
    {
        return -EINVAL;
    }
    if (start_floor < 1 || start_floor > NUM_FLOORS ||
        destination_floor < 1 || destination_floor > NUM_FLOORS ||
        type < 0 || type > 3) {
        return -EINVAL;
    }

    new_passenger = kmalloc(sizeof(*new_passenger), GFP_KERNEL);
    if (!new_passenger) {
        return -ENOMEM;
    }

    new_passenger->type = type;
    new_passenger->start_floor = start_floor;
    new_passenger->destination_floor = destination_floor;
    INIT_LIST_HEAD(&new_passenger->list);

    mutex_lock(&elevator.lock);
    list_add_tail(&new_passenger->list, &elevator.passengers);
    elevator.weight += calculate_weight(type);
    elevator.num_passengers++;
    mutex_unlock(&elevator.lock);

    return 0;
}

static int __init elevator_init(void) {
    struct proc_dir_entry *entry;
    entry = proc_create("elevator", 0, NULL, &elevator_proc_fops);
    if(!entry)
    {
        printk(KERN_ERR "Failed to create proc entry\n");
        return -ENOMEM;
    }
    // Initialize elevator data structure
    mutex_init(&elevator.lock);
    INIT_LIST_HEAD(&elevator.passengers);
    elevator.current_floor = 1;
    elevator.direction = 0;
    elevator.weight = 0;
    elevator.num_passengers = 0;

    // Start elevator thread
    elevator.thread = kthread_run(elevator_function, NULL, "elevator_thread");
    if (IS_ERR(elevator.thread)) {
        // Handle error
        return PTR_ERR(elevator.thread);
    }
    
    return 0;
}

static void __exit elevator_exit(void) {
    // Stop elevator thread
    if (elevator.thread) {
        kthread_stop(elevator.thread);
        elevator.thread = NULL;
    }

    mutex_lock(&elevator.lock);
    
    // Clean up passengers
    struct list_head *pos, *q;
    struct passenger *tmp;

    
    list_for_each_safe(pos, q, &elevator.passengers) {
        tmp = list_entry(pos, struct passenger, list);
        list_del(pos);
        kfree(tmp);
    }
    mutex_unlock(&elevator.lock);
    remove_proc_entry("elevator", NULL);
    mutex_destroy(&elevator.lock);
    printk(KERN_INFO "Elevator module exited\n");
}

static int elevator_proc_show(struct seq_file *m, void *v) {
    struct passenger *passenger;
    struct list_head *pos;

    // Lock to ensure consistency while reading elevator state
    mutex_lock(&elevator.lock);

    // Print the elevator's state
    const char *state = "OFFLINE"; // Default state
    if (elevator.thread) {
        if (list_empty(&elevator.passengers)) {
            state = "IDLE";
        } else {
            state = (elevator.direction == 1) ? "UP" :
                    (elevator.direction == -1) ? "DOWN" : "LOADING";
        }
    }
    seq_printf(m, "Elevator state: %s\n", state);
    seq_printf(m, "Current floor: %d\n", elevator.current_floor);
    seq_printf(m, "Current load: %d lbs\n", elevator.weight / 10); // Convert back to lbs
    seq_printf(m, "Elevator passengers:\n");

    // Iterate over passengers in the elevator
    list_for_each(pos, &elevator.passengers) {
        passenger = list_entry(pos, struct passenger, list);
        char passenger_type = 'X'; // Placeholder for passenger type
        switch (passenger->type) {
            case 0: passenger_type = 'P'; break;
            case 1: passenger_type = 'L'; break;
            case 2: passenger_type = 'B'; break;
            case 3: passenger_type = 'V'; break;
        }
        seq_printf(m, "%c%d ", passenger_type, passenger->destination_floor);
    }
    seq_puts(m, "\n");

    // Placeholder values for waiting and serviced passengers
    seq_printf(m, "Number of passengers waiting: %d\n", 0); // Placeholder
    seq_printf(m, "Number of passengers serviced: %d\n", 0); // Placeholder

    mutex_unlock(&elevator.lock);
    return 0;
}


module_init(elevator_init);
module_exit(elevator_exit);
