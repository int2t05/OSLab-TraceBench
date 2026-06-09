/*
 * 文件作用：实现 TraceBench workload 子进程中的资源压力生成。
 * 设计原因：压力生成必须与父进程采样和 cgroup 管理分离，子进程等待父进程
 * 完成 cgroup 归组后才开始运行，保证采样指标属于本次实验。
 */

#include "tracebench.h"

#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    long long end_ms;
} CpuWorkerArgs;

/*
 * CPU worker 只做可预测的忙循环。
 * 不调用 sleep 是为了制造稳定 CPU 压力；循环体保留 volatile 变量，避免编译器优化为空循环。
 */
static void *cpu_worker_main(void *arg)
{
    CpuWorkerArgs *args = (CpuWorkerArgs *)arg;
    volatile unsigned long long value = 0;

    while (tb_now_millis() < args->end_ms) {
        value = value * 1103515245ULL + 12345ULL;
    }

    return NULL;
}

static int wait_for_start_signal(int start_fd)
{
    char signal_byte;
    ssize_t nread = read(start_fd, &signal_byte, 1);

    close(start_fd);
    if (nread != 1) {
        tb_print_error("workload did not receive start signal");
        return -1;
    }

    return 0;
}

static int run_cpu_workload(const TbConfig *config)
{
    pthread_t *threads;
    CpuWorkerArgs args;
    int started = 0;

    threads = calloc((size_t)config->cpu_workers, sizeof(*threads));
    if (threads == NULL) {
        tb_print_error("failed to allocate CPU worker threads");
        return -1;
    }

    args.end_ms = tb_now_millis() + (long long)config->duration_sec * 1000LL;
    for (int i = 0; i < config->cpu_workers; i++) {
        if (pthread_create(&threads[i], NULL, cpu_worker_main, &args) != 0) {
            tb_print_error("failed to create CPU worker");
            break;
        }
        started++;
    }

    for (int i = 0; i < started; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    return started == config->cpu_workers ? 0 : -1;
}

int tb_run_workload_child(const TbConfig *config, int start_fd)
{
    if (wait_for_start_signal(start_fd) != 0) {
        return -1;
    }

    if (config->profile == TB_PROFILE_CPU) {
        return run_cpu_workload(config);
    }

    tb_print_error("selected workload is not implemented yet");
    return -1;
}
