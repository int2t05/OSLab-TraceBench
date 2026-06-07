/*
 * 文件作用：实现 filesystem 命令行入口，逐行执行批处理文件系统命令。
 * 设计原因：课程要求可用输入文件演示文件系统操作，入口层负责保持命令边界和最终退出码。
 */
#include "filesystem.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_help(const char *program)
{
    printf("usage: %s < tests/fs_commands.txt\n", program);
    printf("commands:\n");
    printf("  mkfs DISK_SIZE BLOCK_SIZE\n");
    printf("  mkdir PATH\n");
    printf("  create PATH\n");
    printf("  write PATH CONTENT\n");
    printf("  read PATH\n");
    printf("  ls PATH\n");
    printf("  delete PATH\n");
    printf("  stat\n");
}

static char *trim(char *line)
{
    char *end;

    while (isspace((unsigned char)*line)) {
        line++;
    }
    if (*line == '\0') {
        return line;
    }
    end = line + strlen(line) - 1;
    while (end > line && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    return line;
}

/*
 * 执行单条已解析命令。
 * 文件系统错误不立即终止，是为了让批处理样例能展示多个错误场景并在最后返回非 0。
 */
static int execute_command(FileSystem *fs, const FsCommand *command, char *error)
{
    char *content = NULL;
    FsNode *dir = NULL;
    int total_blocks;
    int used_blocks;
    int free_blocks;
    int status;

    switch (command->type) {
    case CMD_MKFS:
        status = fs_mkfs(fs, command->disk_size, command->block_size, error, FS_ERROR_LEN);
        if (status == 0) {
            fs_print_ok("mkfs complete");
        }
        return status;
    case CMD_MKDIR:
        status = fs_mkdir(fs, command->path, error, FS_ERROR_LEN);
        if (status == 0) {
            fs_print_ok("directory created");
        }
        return status;
    case CMD_CREATE:
        status = fs_create(fs, command->path, error, FS_ERROR_LEN);
        if (status == 0) {
            fs_print_ok("file created");
        }
        return status;
    case CMD_WRITE:
        status = fs_write(fs, command->path, command->content, error, FS_ERROR_LEN);
        if (status == 0) {
            fs_print_ok("file written");
        }
        return status;
    case CMD_READ:
        status = fs_read(fs, command->path, &content, error, FS_ERROR_LEN);
        if (status == 0) {
            fs_print_content(content);
            free(content);
        }
        return status;
    case CMD_LS:
        status = fs_ls(fs, command->path, &dir, error, FS_ERROR_LEN);
        if (status == 0) {
            fs_print_ls(dir);
        }
        return status;
    case CMD_DELETE:
        status = fs_delete(fs, command->path, error, FS_ERROR_LEN);
        if (status == 0) {
            fs_print_ok("file deleted");
        }
        return status;
    case CMD_STAT:
        status = fs_stat(fs, &total_blocks, &used_blocks, &free_blocks, error, FS_ERROR_LEN);
        if (status == 0) {
            fs_print_stat(total_blocks, used_blocks, free_blocks);
        }
        return status;
    }

    snprintf(error, FS_ERROR_LEN, "unknown command");
    return -1;
}

int main(int argc, char **argv)
{
    FileSystem fs;
    char line_buffer[FS_LINE_LEN];
    int had_error = 0;

    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_help(argv[0]);
        return 0;
    }
    if (argc != 1) {
        fprintf(stderr, "error: invalid arguments\n");
        print_help(argv[0]);
        return 1;
    }

    fs_init(&fs);
    while (fgets(line_buffer, sizeof(line_buffer), stdin) != NULL) {
        FsCommand command;
        char original[FS_LINE_LEN];
        char error[FS_ERROR_LEN];
        char *line;

        line = trim(line_buffer);
        if (*line == '\0') {
            continue;
        }
        snprintf(original, sizeof(original), "%s", line);
        printf("command: %s\n", original);

        if (parse_fs_command(line, &command, error, sizeof(error)) != 0 ||
            execute_command(&fs, &command, error) != 0) {
            fs_print_error(error);
            had_error = 1;
        }
    }

    fs_destroy(&fs);
    return had_error ? 1 : 0;
}
