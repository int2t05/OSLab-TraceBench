/*
 * 文件作用：解析文件系统批处理命令。
 * 设计原因：命令参数数量和 write 内容限制需要在进入文件系统状态修改前统一校验。
 */
#include "filesystem.h"

#include <stdio.h>
#include <string.h>

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error_size > 0) {
        snprintf(error, error_size, "%s", message);
    }
}

static int copy_path(const char *path, char *dest, char *error, size_t error_size)
{
    if (strlen(path) >= FS_PATH_LEN) {
        set_error(error, error_size, "path is too long");
        return -1;
    }
    memcpy(dest, path, strlen(path) + 1);
    return 0;
}

/*
 * 解析单条命令。
 * write 使用额外字符检测拒绝空格内容，因为 PRD 固定文件内容为不含空格字符串。
 */
int parse_fs_command(const char *line, FsCommand *command, char *error, size_t error_size)
{
    char path[FS_LINE_LEN];
    char content[FS_LINE_LEN];
    char extra;

    memset(command, 0, sizeof(*command));

    if (sscanf(line, "mkfs %d %d %c", &command->disk_size, &command->block_size, &extra) == 2) {
        command->type = CMD_MKFS;
        return 0;
    }
    if (sscanf(line, "mkdir %511s %c", path, &extra) == 1) {
        command->type = CMD_MKDIR;
        return copy_path(path, command->path, error, error_size);
    }
    if (sscanf(line, "create %511s %c", path, &extra) == 1) {
        command->type = CMD_CREATE;
        return copy_path(path, command->path, error, error_size);
    }
    if (sscanf(line, "write %511s %511s %c", path, content, &extra) == 2) {
        command->type = CMD_WRITE;
        if (copy_path(path, command->path, error, error_size) != 0) {
            return -1;
        }
        if (strlen(content) >= sizeof(command->content)) {
            set_error(error, error_size, "content is too long");
            return -1;
        }
        memcpy(command->content, content, strlen(content) + 1);
        return 0;
    }
    if (sscanf(line, "read %511s %c", path, &extra) == 1) {
        command->type = CMD_READ;
        return copy_path(path, command->path, error, error_size);
    }
    if (sscanf(line, "ls %511s %c", path, &extra) == 1) {
        command->type = CMD_LS;
        return copy_path(path, command->path, error, error_size);
    }
    if (sscanf(line, "delete %511s %c", path, &extra) == 1) {
        command->type = CMD_DELETE;
        return copy_path(path, command->path, error, error_size);
    }
    if (sscanf(line, "stat %c", &extra) == -1 || strcmp(line, "stat") == 0) {
        command->type = CMD_STAT;
        return 0;
    }

    set_error(error, error_size, "invalid command or argument count");
    return -1;
}
