/*
 * 文件作用：实现 TraceBench workload 子进程中的资源压力生成。
 * 设计原因：压力生成必须与父进程采样和 cgroup 管理分离，子进程等待父进程
 * 完成 cgroup 归组后才开始运行，保证采样指标属于本次实验。
 */

#include "tracebench.h"

#include <stdint.h>
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

static void workload_sleep_millis(long millis)
{
    struct timespec request;

    request.tv_sec = millis / 1000L;
    request.tv_nsec = (millis % 1000L) * 1000000L;
    while (nanosleep(&request, &request) != 0) {
    }
}

/*
 * memory workload 先按页写入整块内存，再周期性重新触碰。
 * 这样做是为了让 malloc 得到的虚拟地址真正映射到物理页，同时避免一次分配后长期空转导致指标不明显。
 */
static int run_memory_workload(const TbConfig *config)
{
    unsigned char *buffer;
    long page_size;
    size_t total_bytes;
    long long end_ms;
    unsigned char value = 1;

    if ((size_t)config->memory_mb > (SIZE_MAX / (1024U * 1024U))) {
        tb_print_error("--memory-mb is too large");
        return -1;
    }
    total_bytes = (size_t)config->memory_mb * 1024U * 1024U;
    buffer = malloc(total_bytes);
    if (buffer == NULL) {
        tb_print_error("failed to allocate memory workload buffer");
        return -1;
    }

    page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        page_size = 4096;
    }

    end_ms = tb_now_millis() + (long long)config->duration_sec * 1000LL;
    while (tb_now_millis() < end_ms) {
        for (size_t offset = 0; offset < total_bytes; offset += (size_t)page_size) {
            buffer[offset] = value;
        }
        buffer[total_bytes - 1] = value;
        value++;
        workload_sleep_millis(50);
    }

    free(buffer);
    return 0;
}

int tb_run_workload_child(const TbConfig *config, int start_fd)
{
    if (wait_for_start_signal(start_fd) != 0) {
        return -1;
    }

    if (config->profile == TB_PROFILE_CPU) {
        return run_cpu_workload(config);
    }
    if (config->profile == TB_PROFILE_MEMORY) {
        return run_memory_workload(config);
    }

    tb_print_error("selected workload is not implemented yet");
    return -1;
}
