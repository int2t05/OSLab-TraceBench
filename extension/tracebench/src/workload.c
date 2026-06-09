/*
 * 文件作用：实现 TraceBench workload 子进程中的资源压力生成。
 * 设计原因：压力生成必须与父进程采样和 cgroup 管理分离，子进程等待父进程
 * 完成 cgroup 归组后才开始运行，保证采样指标属于本次运行。
 */

#include "tracebench.h"

#include <errno.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static int write_io_file_once(const char *path, const unsigned char *buffer, size_t buffer_size,
                              size_t total_bytes)
{
    FILE *file;
    size_t written_total = 0;

    file = fopen(path, "wb");
    if (file == NULL) {
        tb_print_error("failed to open I/O workload file %s: %s", path, strerror(errno));
        return -1;
    }

    while (written_total < total_bytes) {
        size_t remain = total_bytes - written_total;
        size_t chunk = remain < buffer_size ? remain : buffer_size;

        if (fwrite(buffer, 1, chunk, file) != chunk) {
            tb_print_error("failed to write I/O workload file %s", path);
            fclose(file);
            return -1;
        }
        written_total += chunk;
    }

    if (fflush(file) != 0 || fsync(fileno(file)) != 0) {
        tb_print_error("failed to fsync I/O workload file %s: %s", path, strerror(errno));
        fclose(file);
        return -1;
    }
    if (fclose(file) != 0) {
        tb_print_error("failed to close I/O workload file %s: %s", path, strerror(errno));
        return -1;
    }

    return 0;
}

/*
 * I/O workload 覆盖同一个固定临时文件，而不是持续追加。
 * 这样可以制造写入和 fsync 压力，同时避免验证环境磁盘占用无限增长。
 */
static int run_io_workload(const TbConfig *config)
{
    char path[TB_PATH_LEN];
    unsigned char *buffer;
    size_t buffer_size = 64U * 1024U;
    size_t total_bytes;
    long long end_ms;
    int result = 0;

    if ((size_t)config->io_mb > (SIZE_MAX / (1024U * 1024U))) {
        tb_print_error("--io-mb is too large");
        return -1;
    }
    total_bytes = (size_t)config->io_mb * 1024U * 1024U;
    if (tb_join_path(path, sizeof(path), config->output_path, "tracebench_io.tmp") != 0) {
        return -1;
    }

    buffer = malloc(buffer_size);
    if (buffer == NULL) {
        tb_print_error("failed to allocate I/O workload buffer");
        return -1;
    }
    for (size_t i = 0; i < buffer_size; i++) {
        buffer[i] = (unsigned char)(i & 0xffU);
    }

    end_ms = tb_now_millis() + (long long)config->duration_sec * 1000LL;
    while (tb_now_millis() < end_ms) {
        if (write_io_file_once(path, buffer, buffer_size, total_bytes) != 0) {
            result = -1;
            break;
        }
    }

    free(buffer);
    if (remove(path) != 0 && errno != ENOENT) {
        tb_print_error("failed to remove I/O workload file %s: %s", path, strerror(errno));
        return -1;
    }

    return result;
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
    if (config->profile == TB_PROFILE_IO) {
        return run_io_workload(config);
    }

    tb_print_error("selected workload is not implemented yet");
    return -1;
}
