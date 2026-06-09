/*
 * 文件作用：实现 oslab_monitor Linux 内核模块并创建 /proc/oslab_monitor 接口。
 * 设计原因：该模块验证真实 Linux 内核运行态观测路径，单文件实现便于定位生命周期和 /proc 逻辑。
 */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/pid.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/sched/mm.h>
#include <linux/sched/signal.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/utsname.h>

#define PROC_DIR_NAME "oslab_monitor"
#define PROC_OVERVIEW_NAME "overview"
#define PROC_TASKS_NAME "tasks"
#define PROC_PID_NAME "pid"
#define PID_INPUT_MAX 31

static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *overview_entry;
static struct proc_dir_entry *tasks_entry;
static struct proc_dir_entry *pid_entry;
static DEFINE_MUTEX(target_pid_lock);
static int target_pid = 1;

static const char *task_state_text(unsigned int state)
{
    if (state == TASK_RUNNING) {
        return "R";
    }
    if (state & TASK_INTERRUPTIBLE) {
        return "S";
    }
    if (state & TASK_UNINTERRUPTIBLE) {
        return "D";
    }
    if (state & __TASK_STOPPED) {
        return "T";
    }
    if (state & EXIT_ZOMBIE) {
        return "Z";
    }
    return "?";
}

static const char *policy_text(int policy)
{
    switch (policy) {
    case SCHED_NORMAL:
        return "NORMAL";
    case SCHED_FIFO:
        return "FIFO";
    case SCHED_RR:
        return "RR";
    case SCHED_BATCH:
        return "BATCH";
    case SCHED_IDLE:
        return "IDLE";
    default:
        return "OTHER";
    }
}

/*
 * 读取任务 RSS。
 * get_task_mm 会为 mm_struct 增加引用，读取后必须 mmput；内核线程没有 mm 时返回 0。
 */
static unsigned long task_rss_kb(struct task_struct *task)
{
    struct mm_struct *mm;
    unsigned long rss = 0;

    mm = get_task_mm(task);
    if (mm == NULL) {
        return 0;
    }
    rss = get_mm_rss(mm) << (PAGE_SHIFT - 10);
    mmput(mm);
    return rss;
}

static void count_task_state(struct task_struct *task, unsigned long *running,
                             unsigned long *sleeping, unsigned long *stopped,
                             unsigned long *zombie)
{
    unsigned int state = READ_ONCE(task->__state);
    int exit_state = READ_ONCE(task->exit_state);

    if (state == TASK_RUNNING) {
        (*running)++;
    } else if (state & (__TASK_STOPPED | __TASK_TRACED)) {
        (*stopped)++;
    } else {
        (*sleeping)++;
    }

    if (exit_state & EXIT_ZOMBIE) {
        (*zombie)++;
    }
}

/*
 * 输出系统概览。
 * 使用 seq_file 是为了让 /proc 读取由内核框架管理缓冲区，避免固定小缓冲区截断。
 */
static int overview_show(struct seq_file *m, void *v)
{
    struct task_struct *task;
    struct sysinfo info;
    unsigned long total = 0;
    unsigned long running = 0;
    unsigned long sleeping = 0;
    unsigned long stopped = 0;
    unsigned long zombie = 0;

    for_each_process(task) {
        total++;
        count_task_state(task, &running, &sleeping, &stopped, &zombie);
    }

    si_meminfo(&info);
    seq_puts(m, "module: oslab_monitor\n");
    seq_printf(m, "kernel: %s\n", init_uts_ns.name.release);
    seq_printf(m, "total_tasks: %lu\n", total);
    seq_printf(m, "running_tasks: %lu\n", running);
    seq_printf(m, "sleeping_tasks: %lu\n", sleeping);
    seq_printf(m, "stopped_tasks: %lu\n", stopped);
    seq_printf(m, "zombie_tasks: %lu\n", zombie);
    seq_printf(m, "mem_total_kb: %lu\n", info.totalram << (PAGE_SHIFT - 10));
    seq_printf(m, "mem_free_kb: %lu\n", info.freeram << (PAGE_SHIFT - 10));
    seq_printf(m, "mem_available_kb: %lu\n", si_mem_available() << (PAGE_SHIFT - 10));
    seq_printf(m, "read_time_jiffies: %lu\n", jiffies);
    return 0;
}

static int overview_open(struct inode *inode, struct file *file)
{
    return single_open(file, overview_show, NULL);
}

static const struct proc_ops overview_ops = {
    .proc_open = overview_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/*
 * 输出进程列表。
 * 遍历全部进程且不设置固定数量上限，长输出由 seq_file 分段处理。
 */
static int tasks_show(struct seq_file *m, void *v)
{
    struct task_struct *task;

    seq_puts(m,
             "PID     COMM            STATE   POLICY  PRIO  NICE  THREADS  RSS_KB  MIN_FLT  MAJ_FLT\n");
    for_each_process(task) {
        seq_printf(m, "%-7d %-15s %-7s %-7s %-5d %-5d %-8d %-7lu %-8s %-8s\n",
                   task_pid_nr(task), task->comm,
                   task_state_text(READ_ONCE(task->__state) | READ_ONCE(task->exit_state)),
                   policy_text(task->policy), task->prio, task_nice(task), get_nr_threads(task),
                   task_rss_kb(task), "N/A", "N/A");
    }
    return 0;
}

static int tasks_open(struct inode *inode, struct file *file)
{
    return single_open(file, tasks_show, NULL);
}

static const struct proc_ops tasks_ops = {
    .proc_open = tasks_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static struct task_struct *find_task_by_target_pid(int pid)
{
    struct pid *pid_struct;
    struct task_struct *task;

    pid_struct = find_get_pid(pid);
    if (pid_struct == NULL) {
        return NULL;
    }
    task = get_pid_task(pid_struct, PIDTYPE_PID);
    put_pid(pid_struct);
    return task;
}

/*
 * 输出指定 PID 详情。
 * 目标 PID 在读前复制到局部变量，避免长时间持锁影响写入路径。
 */
static int pid_show(struct seq_file *m, void *v)
{
    struct task_struct *task;
    int pid;

    mutex_lock(&target_pid_lock);
    pid = target_pid;
    mutex_unlock(&target_pid_lock);

    task = find_task_by_target_pid(pid);
    if (task == NULL) {
        seq_printf(m, "pid: %d\n", pid);
        seq_puts(m, "not found\n");
        return 0;
    }

    seq_printf(m, "pid: %d\n", task_pid_nr(task));
    seq_printf(m, "comm: %s\n", task->comm);
    seq_printf(m, "state: %s\n",
               task_state_text(READ_ONCE(task->__state) | READ_ONCE(task->exit_state)));
    seq_printf(m, "ppid: %d\n", task_ppid_nr(task));
    seq_printf(m, "policy: %s\n", policy_text(task->policy));
    seq_printf(m, "prio: %d\n", task->prio);
    seq_printf(m, "nice: %d\n", task_nice(task));
    seq_printf(m, "threads: %d\n", get_nr_threads(task));
    seq_printf(m, "rss_kb: %lu\n", task_rss_kb(task));
    put_task_struct(task);
    return 0;
}

static int pid_open(struct inode *inode, struct file *file)
{
    return single_open(file, pid_show, NULL);
}

/*
 * 写入目标 PID。
 * 输入长度固定上限并使用 copy_from_user + kstrtoint，避免用户输入导致内核缓冲区越界。
 */
static ssize_t pid_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    char input[PID_INPUT_MAX + 1];
    int parsed_pid;

    if (count == 0 || count > PID_INPUT_MAX) {
        return -EINVAL;
    }
    if (copy_from_user(input, buffer, count) != 0) {
        return -EFAULT;
    }
    input[count] = '\0';
    if (kstrtoint(input, 10, &parsed_pid) != 0 || parsed_pid <= 0) {
        return -EINVAL;
    }

    mutex_lock(&target_pid_lock);
    target_pid = parsed_pid;
    mutex_unlock(&target_pid_lock);
    return count;
}

static const struct proc_ops pid_ops = {
    .proc_open = pid_open,
    .proc_read = seq_read,
    .proc_write = pid_write,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static void remove_proc_entries(void)
{
    if (pid_entry != NULL) {
        proc_remove(pid_entry);
        pid_entry = NULL;
    }
    if (tasks_entry != NULL) {
        proc_remove(tasks_entry);
        tasks_entry = NULL;
    }
    if (overview_entry != NULL) {
        proc_remove(overview_entry);
        overview_entry = NULL;
    }
    if (proc_dir != NULL) {
        proc_remove(proc_dir);
        proc_dir = NULL;
    }
}

/*
 * 模块加载入口。
 * 任一 proc 节点创建失败都会回滚已创建节点，避免部分初始化残留。
 */
static int __init oslab_monitor_init(void)
{
    proc_dir = proc_mkdir(PROC_DIR_NAME, NULL);
    if (proc_dir == NULL) {
        return -ENOMEM;
    }

    overview_entry = proc_create(PROC_OVERVIEW_NAME, 0444, proc_dir, &overview_ops);
    tasks_entry = proc_create(PROC_TASKS_NAME, 0444, proc_dir, &tasks_ops);
    pid_entry = proc_create(PROC_PID_NAME, 0644, proc_dir, &pid_ops);
    if (overview_entry == NULL || tasks_entry == NULL || pid_entry == NULL) {
        remove_proc_entries();
        return -ENOMEM;
    }

    pr_info("oslab_monitor: loaded\n");
    return 0;
}

static void __exit oslab_monitor_exit(void)
{
    remove_proc_entries();
    pr_info("oslab_monitor: unloaded\n");
}

module_init(oslab_monitor_init);
module_exit(oslab_monitor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OS-Design");
MODULE_DESCRIPTION("OS lab process and memory monitor via /proc");
