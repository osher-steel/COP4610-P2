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
MODULE_AUTHOR("Your Name");
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

static int elevator_proc_show(struct seq_file *m, void *v) {
    mutex_lock(&elevator.lock);
    seq_printf(m, "Current Floor: %d\n", elevator.current_floor);
    seq_printf(m, "Direction: %d\n", elevator.direction);
    seq_printf(m, "Weight: %d\n", elevator.weight);
    seq_printf(m, "Number of Passengers: %d\n", elevator.num_passengers);
    mutex_unlock(&elevator.lock);
    return 0;
}

static int elevator_proc_open(struct inode *inode, struct  file *file) {
  return single_open(file, elevator_proc_show, NULL);
}

static const struct proc_ops elevator_proc_ops = {
    .proc_open = elevator_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static void elevator_create_proc(void) {
    proc_create("elevator", 0, NULL, &elevator_proc_ops);
}


static void elevator_remove_proc(void) {
    remove_proc_entry("elevator", NULL);
}


// Elevator thread function
int elevator_function(void *data) {
    printk(KERN_INFO "Elevator thread started\n");
    while (!kthread_should_stop()) {
        printk(KERN_INFO "Elevator looping\n");
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
            printk(KERN_INFO "No passengers sitting idle\n");
            msleep(1000); // Idle time
        }

        mutex_unlock(&elevator.lock);
        // Yield processor to prevent starvation of other processes/threads
        schedule();
    }
    printk(KERN_INFO "Elevator thread is stopping\n");
    return 0;
}


// Simulated system call to add a passenger
int issue_request(int start_floor, int destination_floor, int type) {
    printk(KERN_INFO "New passenger");
    struct passenger *new_passenger;
    int new_weight;
    new_weight = calculate_weight(type);
    if ((elevator.weight + new_weight > MAX_WEIGHT) || elevator.num_passengers >= MAX_PASSENGERS)
    {
        printk(KERN_ERR "Elevator is full");
        return -EINVAL;
    }
    if (start_floor < 1 || start_floor > NUM_FLOORS ||
        destination_floor < 1 || destination_floor > NUM_FLOORS ||
        type < 0 || type > 3) {
        printk(KERN_ERR "Invalid request");
        return -EINVAL;
    }

    new_passenger = kmalloc(sizeof(*new_passenger), GFP_KERNEL);
    if (!new_passenger) {
        printk(KERN_ERR "Failed to allocate memory for new passenger\n");
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

    printk(KERN_INFO "New passenger added: Type=%d, Start=%d, Destination=%d, TotalWeight=%d, NumPassengers=%d\n", type, start_floor, destination_floor, elevator.weight, elevator.num_passengers);

    return 0;
}

static int __init elevator_init(void) {
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
    printk(KERN_ERR "Failed to create elevator thread: error %ld\n", PTR_ERR(elevator.thread));
    return PTR_ERR(elevator.thread);
} else {
    printk(KERN_INFO "Elevator thread created successfully\n");
}
    elevator_create_proc();
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
    mutex_destroy(&elevator.lock);
    elevator_remove_proc();
    printk(KERN_INFO "Elevator module exited\n");
}

module_init(elevator_init);
module_exit(elevator_exit);
