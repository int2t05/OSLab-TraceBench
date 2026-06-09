/*
 * 文件作用：声明内存型文件系统模型模块的共享类型和函数。
 * 设计原因：命令解析、目录树操作和输出分文件实现，共用头文件可以保持接口稳定。
 */
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stddef.h>

#define FS_NAME_LEN 32
#define FS_PATH_LEN 256
#define FS_LINE_LEN 512
#define FS_ERROR_LEN 256

typedef enum {
    NODE_FILE,
    NODE_DIR
} NodeType;

typedef struct FsNode {
    char name[FS_NAME_LEN];
    NodeType type;
    int size;
    int *blocks;
    int block_count;
    struct FsNode *parent;
    struct FsNode **children;
    int child_count;
    int child_capacity;
} FsNode;

typedef struct {
    int disk_size;
    int block_size;
    int block_count;
    unsigned char *block_used;
    char **block_data;
    FsNode *root;
} FileSystem;

typedef enum {
    CMD_MKFS,
    CMD_MKDIR,
    CMD_CREATE,
    CMD_WRITE,
    CMD_READ,
    CMD_LS,
    CMD_DELETE,
    CMD_STAT
} FsCommandType;

typedef struct {
    FsCommandType type;
    int disk_size;
    int block_size;
    char path[FS_PATH_LEN];
    char content[FS_LINE_LEN];
} FsCommand;

int parse_fs_command(const char *line, FsCommand *command, char *error, size_t error_size);

void fs_init(FileSystem *fs);
void fs_destroy(FileSystem *fs);
int fs_mkfs(FileSystem *fs, int disk_size, int block_size, char *error, size_t error_size);
int fs_mkdir(FileSystem *fs, const char *path, char *error, size_t error_size);
int fs_create(FileSystem *fs, const char *path, char *error, size_t error_size);
int fs_write(FileSystem *fs, const char *path, const char *content, char *error,
             size_t error_size);
int fs_read(FileSystem *fs, const char *path, char **content, char *error, size_t error_size);
int fs_ls(FileSystem *fs, const char *path, FsNode **dir, char *error, size_t error_size);
int fs_delete(FileSystem *fs, const char *path, char *error, size_t error_size);
int fs_stat(FileSystem *fs, int *total_blocks, int *used_blocks, int *free_blocks, char *error,
            size_t error_size);

void fs_print_ok(const char *message);
void fs_print_error(const char *message);
void fs_print_content(const char *content);
void fs_print_ls(const FsNode *dir);
void fs_print_stat(int total_blocks, int used_blocks, int free_blocks);

#endif
