#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group #");
MODULE_DESCRIPTION("Timer Module");
MODULE_VERSION("1.0");

#define ENTRY_NAME "my_timer"
#define PERMS 0666
#define PARENT NULL

#define BUF_LEN 100
static struct proc_dir_entry* proc_entry;
static char msg[BUF_LEN];
static int procfs_buf_len;
struct timespec64 last_time;

static ssize_t procfile_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
    printk(KERN_INFO "proc_read\n");

    struct timespec64 current_time;
    ktime_get_real_ts64(&current_time);

    procfs_buf_len = snprintf(msg, sizeof(msg), "current time: %ld.%09ld\n",(long) current_time.tv_sec, current_time.tv_nsec);

    u64 elapsed_time = ktime_to_ns(ktime_sub(ktime_set(current_time.tv_sec, current_time.tv_nsec), ktime_set(last_time.tv_sec,last_time.tv_nsec)));

    if(elapsed_time > 0)
                procfs_buf_len += snprintf(msg + procfs_buf_len, sizeof(msg) - procfs_buf_len, "elapsed time: %lld.%09lld\n", elapsed_time / NSEC_PER>

    if (*ppos > 0 || count < procfs_buf_len)
        return 0;

    if (copy_to_user(ubuf, msg, procfs_buf_len))
        return -EFAULT;

    *ppos = *ppos + procfs_buf_len;

    last_time =  current_time;

    printk(KERN_INFO "gave to user: %s", msg);

    return procfs_buf_len;
};

static const struct proc_ops procfile_fops = {
        .proc_read = procfile_read,
};

static int __init timer_init(void){
        proc_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &procfile_fops);
        if (proc_entry == NULL) {
                printk(KERN_ERR "Failed to create /proc/my_timer entry\n");
                return -ENOMEM;
        }
        else printk(KERN_INFO "Successfully created /proc/my_timer entry/n");
        ktime_get_real_ts64(&last_time);

        return 0;
};

static void __exit timer_exit(void){
        proc_remove(proc_entry);
};

module_init(timer_init);
module_exit(timer_exit);
