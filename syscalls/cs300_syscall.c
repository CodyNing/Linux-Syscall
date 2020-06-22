#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/limits.h>
#include <linux/stddef.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <asm/errno.h>

#define ANCESTOR_NAME_LEN 16

struct array_stats_s
{
    long min;
    long max;
    long sum;
};

SYSCALL_DEFINE3(
    array_stats,                   /* syscall name */
    struct array_stats_s *, stats, /* where to write stats */
    long *, data,                  /* data to process */
    long, size)                    /* # values in data */
{
    //i for loop, item for temp copy of data[i]
    long i = 0, item = 0;
    //n_bytes to store result from copy function, reuseable
    unsigned long n_bytes;
    //m_stats is a local result struct, will be copied to user pointer when ready
    //set min to LONG_MAX and max to LONG_MIN, so we could compare all possible values
    struct array_stats_s m_stats = {LONG_MAX, LONG_MIN, 0};

    printk("size: %ld", size);

    //return -EINVAL if size <=0 (requirement)
    if (size <= 0)
    {
        return -EINVAL;
    }
    //return -EFAULT if unable to access stats or data (null pointer) (requirement)
    if (data == NULL || stats == NULL)
    {
        return -EFAULT;
    }

    for (i = 0; i < size; ++i)
    {

        //try to copy from data[i]
        n_bytes = copy_from_user(&item, data + i, sizeof(long));

        //return -EFAULT if unable to access data (bad pointer) (requirement)
        if (n_bytes > 0)
        {
            return -EFAULT;
        }

        //counting items
        m_stats.sum += item;
        
        //compare logics
        if (item > m_stats.max)
        {
            m_stats.max = item;
        }
        if (item < m_stats.min)
        {
            m_stats.min = item;
        }
    }

    //try to copy local result struct to stats
    n_bytes = copy_to_user(stats, &m_stats, sizeof(struct array_stats_s));

    //return -EFAULT if unable to access stats (bad pointer) (requirement)
    if (n_bytes > 0)
    {
        return -EFAULT;
    }

    //return 0 when successful (requirement)
    return 0;
}

struct process_info
{
    long pid;                     /* Process ID */
    char name[ANCESTOR_NAME_LEN]; /* Program name of process */
    long state;                   /* Current process state */
    long uid;                     /* User ID of process owner */
    long nvcsw;                   /* # voluntary context switches */
    long nivcsw;                  /* # involuntary context switches */
    long num_children;            /* # children process has */
    long num_siblings;            /* # sibling process has */
};

SYSCALL_DEFINE3(
    process_ancestors,                 /* syscall name for macro */
    struct process_info *, info_array, /* array of process info strct */
    long, size,                        /* size of the array */
    long *, num_filled)                /* # elements written to array */
{
    //i for loop, m_num_filled is a local temp result for num_filled
    long i = 0, m_num_filled = 0;
    //n_bytes to store result from copy function, reuseable
    unsigned long n_bytes;
    //cur is a pointer to the current process
    struct task_struct *cur = current;
    //cursor is a pointer for looping task list
    struct task_struct *cursor;
    //cur_info is local temp process_info to store the current process's info
    //it can be resued in the loop
    struct process_info cur_info;

    //return -EINVAL if size <=0 (requirement)
    if (size <= 0)
    {
        return -EINVAL;
    }

    //return -EFAULT if unable to access info_array or num_filled (null pointer) (requirement)
    if (info_array == NULL || num_filled == NULL)
    {
        return -EFAULT;
    }

    //checking current process is actually available.
    //Since this syscall is being invoked, this case should never happen. Acting safe.
    if (cur == NULL)
    {
        return -EFAULT;
    }

    //looping through all ancestors until the info_array is full
    //always record the current process
    for (i = 0; i < size; ++i)
    {
        //Process ID
        cur_info.pid = cur->pid;
        //Program name of process
        strcpy(cur_info.name, cur->comm);
        //Current process state
        cur_info.state = cur->state;
        //voluntary context switches
        cur_info.nvcsw = cur->nvcsw;
        //involuntary context switches
        cur_info.nivcsw = cur->nivcsw;

        //null pointer check before accessing the pointer
        if (cur->cred)
        {
            //User ID of process owner
            cur_info.uid = cur->cred->uid.val;
        }
        else
        {
            //set the user ID to -1 if encounter null pointer
            cur_info.uid = -1;
        }

        printk("My PID %ld\nMy Name %s\nMy state %ld\nMy nvcsw %ld\nMy nivcsw %ld\nMy uid %ld\n",
            cur_info.pid,
            cur_info.name,
            cur_info.state,
            cur_info.nvcsw,
            cur_info.nivcsw,
            cur_info.uid);

        //start with 0 child
        cur_info.num_children = 0;

        //use macro to loop through all child
        list_for_each_entry(cursor, &cur->children, children)
        {
            printk("    My children PID %d = '%s'; \n",
                   cursor->pid,
                   cursor->comm);
            ++(cur_info.num_children);
        }

        printk("My num_children %ld\n", cur_info.num_children);
        
        //start with 0 sibling
        cur_info.num_siblings = 0;

        //use macro to loop through all siblings
        list_for_each_entry(cursor, &cur->sibling, sibling)
        {
            printk("    My sibling PID %d = '%s'; \n",
                   cursor->pid,
                   cursor->comm);
            ++(cur_info.num_siblings);
        }

        printk("My num_siblings %ld\n", cur_info.num_siblings);

        //try to copy the current process info to info_array[i]
        n_bytes = copy_to_user(info_array + i, &cur_info, sizeof(struct process_info));

        //return -EFAULT if unable to access info_array (bad pointer) (requirement)
        if (n_bytes > 0)
        {
            return -EFAULT;
        }

        //copy done, increment counter
        ++m_num_filled;

        //breakout condition 2: parent is itself
        if (cur == cur->parent)
        {
            break;
        }

        //not breaking out, set the current process to its parent
        cur = cur->parent;
    }

    //try to copy the num_filled counter to user 
    n_bytes = copy_to_user(num_filled, &m_num_filled, sizeof(long));

    //return -EFAULT if unable to access num_filled (bad pointer) (requirement)
    if (n_bytes > 0)
    {
        return -EFAULT;
    }

    //return 0 when successful (requirement)
    return 0;
}