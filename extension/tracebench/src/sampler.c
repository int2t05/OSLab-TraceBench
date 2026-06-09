/*
 * 文件作用：保留 TraceBench PSI、cgroup 和 oslab_monitor 采样模块。
 * 设计原因：采样与 CLI 和 workload 解耦，后续任务可以在这里逐步实现稳定 CSV 输出。
 */

#include "tracebench.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_psi_line(const char *line, const char *prefix, TbPsiLine *psi_line)
{
    const char *avg10_text;
    const char *avg60_text;
    const char *avg300_text;
    const char *total_text;
    char *end = NULL;

    if (strncmp(line, prefix, strlen(prefix)) != 0) {
        return 0;
    }

    avg10_text = strstr(line, "avg10=");
    avg60_text = strstr(line, "avg60=");
    avg300_text = strstr(line, "avg300=");
    total_text = strstr(line, "total=");
    if (avg10_text == NULL || avg60_text == NULL || avg300_text == NULL || total_text == NULL) {
        tb_print_error("invalid PSI line: %s", line);
        return -1;
    }

    psi_line->avg10 = strtod(avg10_text + 6, &end);
    if (end == avg10_text + 6) {
        tb_print_error("invalid PSI avg10 value");
        return -1;
    }
    psi_line->avg60 = strtod(avg60_text + 6, &end);
    if (end == avg60_text + 6) {
        tb_print_error("invalid PSI avg60 value");
        return -1;
    }
    psi_line->avg300 = strtod(avg300_text + 7, &end);
    if (end == avg300_text + 7) {
        tb_print_error("invalid PSI avg300 value");
        return -1;
    }
    psi_line->total = strtoull(total_text + 6, &end, 10);
    if (end == total_text + 6) {
        tb_print_error("invalid PSI total value");
        return -1;
    }
    psi_line->present = 1;

    return 1;
}

static int parse_psi_text(const char *text, TbPsiResource *resource)
{
    char copy[TB_LINE_LEN * 4];
    char *line;
    char *saveptr = NULL;

    memset(resource, 0, sizeof(*resource));
    if ((int)strlen(text) >= (int)sizeof(copy)) {
        tb_print_error("PSI text is too large");
        return -1;
    }
    snprintf(copy, sizeof(copy), "%s", text);

    line = strtok_r(copy, "\n", &saveptr);
    while (line != NULL) {
        int parsed;

        parsed = parse_psi_line(line, "some ", &resource->some);
        if (parsed < 0) {
            return -1;
        }
        parsed = parse_psi_line(line, "full ", &resource->full);
        if (parsed < 0) {
            return -1;
        }
        line = strtok_r(NULL, "\n", &saveptr);
    }

    if (!resource->some.present) {
        tb_print_error("PSI some line is missing");
        return -1;
    }

    return 0;
}

static int read_psi_resource(const char *path, TbPsiResource *resource)
{
    char text[TB_LINE_LEN * 4];

    if (tb_read_text_file(path, text, sizeof(text)) != 0) {
        return -1;
    }

    return parse_psi_text(text, resource);
}

int tb_read_psi_snapshot(TbPsiSnapshot *snapshot)
{
    memset(snapshot, 0, sizeof(*snapshot));

    if (read_psi_resource("/proc/pressure/cpu", &snapshot->cpu) != 0) {
        return -1;
    }
    if (read_psi_resource("/proc/pressure/memory", &snapshot->memory) != 0) {
        return -1;
    }
    if (read_psi_resource("/proc/pressure/io", &snapshot->io) != 0) {
        return -1;
    }

    return 0;
}

int tb_read_oslab_snapshot(TbOslabSnapshot *snapshot)
{
    (void)snapshot;
    tb_print_error("oslab_monitor sampling is not implemented yet");
    return -1;
}

int tb_write_csv_header(FILE *out)
{
    fprintf(out,
            "sample_index,elapsed_ms,profile,"
            "cpu_some_avg10,cpu_some_avg60,cpu_some_avg300,cpu_some_total,"
            "cpu_full_avg10,cpu_full_avg60,cpu_full_avg300,cpu_full_total,"
            "memory_some_avg10,memory_some_avg60,memory_some_avg300,memory_some_total,"
            "memory_full_avg10,memory_full_avg60,memory_full_avg300,memory_full_total,"
            "io_some_avg10,io_some_avg60,io_some_avg300,io_some_total,"
            "io_full_avg10,io_full_avg60,io_full_avg300,io_full_total\n");
    return ferror(out) ? -1 : 0;
}

static void write_psi_line(FILE *out, const TbPsiLine *line)
{
    if (line->present) {
        fprintf(out, "%.2f,%.2f,%.2f,%llu",
                line->avg10,
                line->avg60,
                line->avg300,
                line->total);
    } else {
        fprintf(out, "NA,NA,NA,NA");
    }
}

int tb_write_csv_sample(FILE *out, const TbConfig *config, const TbCgroup *cgroup, const TbSample *sample)
{
    (void)cgroup;

    fprintf(out, "%d,%lld,%s,",
            sample->sample_index,
            sample->elapsed_ms,
            tb_profile_name(config->profile));
    write_psi_line(out, &sample->psi.cpu.some);
    fprintf(out, ",");
    write_psi_line(out, &sample->psi.cpu.full);
    fprintf(out, ",");
    write_psi_line(out, &sample->psi.memory.some);
    fprintf(out, ",");
    write_psi_line(out, &sample->psi.memory.full);
    fprintf(out, ",");
    write_psi_line(out, &sample->psi.io.some);
    fprintf(out, ",");
    write_psi_line(out, &sample->psi.io.full);
    fprintf(out, "\n");

    return ferror(out) ? -1 : 0;
}
