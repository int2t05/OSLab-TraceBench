/*
 * 文件作用：实现内存型虚拟磁盘、目录树、块位图和文件操作。
 * 设计原因：课程设计只要求运行期模拟，目录树加块位图能直接展示路径管理和空闲块管理。
 */
#include "filesystem.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_error(char *error, size_t error_size, const char *message)
{
    if (error_size > 0) {
        snprintf(error, error_size, "%s", message);
    }
}

void fs_init(FileSystem *fs)
{
    memset(fs, 0, sizeof(*fs));
}

static FsNode *create_node(const char *name, NodeType type, FsNode *parent)
{
    FsNode *node = calloc(1, sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    snprintf(node->name, sizeof(node->name), "%s", name);
    node->type = type;
    node->parent = parent;
    return node;
}

static void free_node(FileSystem *fs, FsNode *node)
{
    int i;

    if (node == NULL) {
        return;
    }
    for (i = 0; i < node->child_count; i++) {
        free_node(fs, node->children[i]);
    }
    if (node->type == NODE_FILE) {
        for (i = 0; i < node->block_count; i++) {
            int block = node->blocks[i];
            if (block >= 0 && block < fs->block_count) {
                fs->block_used[block] = 0;
                free(fs->block_data[block]);
                fs->block_data[block] = NULL;
            }
        }
        free(node->blocks);
    }
    free(node->children);
    free(node);
}

void fs_destroy(FileSystem *fs)
{
    int i;

    free_node(fs, fs->root);
    fs->root = NULL;
    if (fs->block_data != NULL) {
        for (i = 0; i < fs->block_count; i++) {
            free(fs->block_data[i]);
        }
    }
    free(fs->block_data);
    free(fs->block_used);
    fs_init(fs);
}

static int require_mounted(const FileSystem *fs, char *error, size_t error_size)
{
    if (fs->root == NULL) {
        set_error(error, error_size, "filesystem is not initialized");
        return -1;
    }
    return 0;
}

static int valid_name(const char *name)
{
    size_t i;
    size_t length = strlen(name);

    if (length == 0 || length >= FS_NAME_LEN) {
        return 0;
    }
    for (i = 0; i < length; i++) {
        unsigned char ch = (unsigned char)name[i];
        if (!(isalnum(ch) || ch == '_' || ch == '-' || ch == '.')) {
            return 0;
        }
    }
    return 1;
}

static FsNode *find_child(FsNode *dir, const char *name)
{
    int i;

    if (dir == NULL || dir->type != NODE_DIR) {
        return NULL;
    }
    for (i = 0; i < dir->child_count; i++) {
        if (strcmp(dir->children[i]->name, name) == 0) {
            return dir->children[i];
        }
    }
    return NULL;
}

static int add_child(FsNode *parent, FsNode *child)
{
    FsNode **new_children;
    int new_capacity;

    if (parent->child_count == parent->child_capacity) {
        new_capacity = parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
        new_children = realloc(parent->children, sizeof(FsNode *) * (size_t)new_capacity);
        if (new_children == NULL) {
            return -1;
        }
        parent->children = new_children;
        parent->child_capacity = new_capacity;
    }
    parent->children[parent->child_count++] = child;
    return 0;
}

static void remove_child(FsNode *parent, FsNode *child)
{
    int i;

    for (i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            int j;
            for (j = i + 1; j < parent->child_count; j++) {
                parent->children[j - 1] = parent->children[j];
            }
            parent->child_count--;
            return;
        }
    }
}

static int split_parent_path(const char *path, char *parent_path, char *name, char *error,
                             size_t error_size)
{
    const char *slash;
    size_t parent_len;

    if (path[0] != '/' || strlen(path) >= FS_PATH_LEN) {
        set_error(error, error_size, "path must be absolute and shorter than 256 characters");
        return -1;
    }
    if (strcmp(path, "/") == 0) {
        set_error(error, error_size, "root cannot be used as a file or directory name");
        return -1;
    }
    slash = strrchr(path, '/');
    if (slash == NULL || *(slash + 1) == '\0') {
        set_error(error, error_size, "path must not end with slash");
        return -1;
    }
    if (!valid_name(slash + 1)) {
        set_error(error, error_size, "invalid file or directory name");
        return -1;
    }

    memcpy(name, slash + 1, strlen(slash + 1) + 1);
    parent_len = (size_t)(slash - path);
    if (parent_len == 0) {
        snprintf(parent_path, FS_PATH_LEN, "/");
    } else {
        memcpy(parent_path, path, parent_len);
        parent_path[parent_len] = '\0';
    }
    return 0;
}

/*
 * 按绝对路径查找节点。
 * 每一级都校验名称，是为了让过长或非法路径在进入目录树前被拒绝。
 */
static int resolve_path(FileSystem *fs, const char *path, FsNode **node, char *error,
                        size_t error_size)
{
    FsNode *current;
    size_t i = 1;

    if (require_mounted(fs, error, error_size) != 0) {
        return -1;
    }
    if (path[0] != '/' || strlen(path) >= FS_PATH_LEN) {
        set_error(error, error_size, "path must be absolute and shorter than 256 characters");
        return -1;
    }
    if (strcmp(path, "/") == 0) {
        *node = fs->root;
        return 0;
    }
    if (path[strlen(path) - 1] == '/') {
        set_error(error, error_size, "path must not end with slash");
        return -1;
    }

    current = fs->root;
    while (path[i] != '\0') {
        char part[FS_NAME_LEN];
        size_t start = i;
        size_t length;

        while (path[i] != '\0' && path[i] != '/') {
            i++;
        }
        length = i - start;
        if (length == 0 || length >= FS_NAME_LEN) {
            set_error(error, error_size, "invalid path component");
            return -1;
        }
        memcpy(part, path + start, length);
        part[length] = '\0';
        if (!valid_name(part)) {
            set_error(error, error_size, "invalid path component");
            return -1;
        }
        current = find_child(current, part);
        if (current == NULL) {
            set_error(error, error_size, "path does not exist");
            return -1;
        }
        if (path[i] == '/') {
            i++;
        }
    }

    *node = current;
    return 0;
}

static int resolve_parent(FileSystem *fs, const char *path, FsNode **parent, char *name,
                          char *error, size_t error_size)
{
    char parent_path[FS_PATH_LEN];

    if (split_parent_path(path, parent_path, name, error, error_size) != 0) {
        return -1;
    }
    if (resolve_path(fs, parent_path, parent, error, error_size) != 0) {
        return -1;
    }
    if ((*parent)->type != NODE_DIR) {
        set_error(error, error_size, "parent path is not a directory");
        return -1;
    }
    return 0;
}

int fs_mkfs(FileSystem *fs, int disk_size, int block_size, char *error, size_t error_size)
{
    FsNode *root;

    if (disk_size <= 0 || block_size <= 0 || disk_size % block_size != 0) {
        set_error(error, error_size, "disk size must be divisible by positive block size");
        return -1;
    }

    fs_destroy(fs);
    fs->disk_size = disk_size;
    fs->block_size = block_size;
    fs->block_count = disk_size / block_size;
    fs->block_used = calloc((size_t)fs->block_count, sizeof(unsigned char));
    fs->block_data = calloc((size_t)fs->block_count, sizeof(char *));
    root = create_node("/", NODE_DIR, NULL);
    if (fs->block_used == NULL || fs->block_data == NULL || root == NULL) {
        free(root);
        fs_destroy(fs);
        set_error(error, error_size, "failed to allocate filesystem");
        return -1;
    }
    fs->root = root;
    return 0;
}

static int create_child(FileSystem *fs, const char *path, NodeType type, char *error,
                        size_t error_size)
{
    FsNode *parent;
    FsNode *child;
    char name[FS_NAME_LEN];

    if (resolve_parent(fs, path, &parent, name, error, error_size) != 0) {
        return -1;
    }
    if (find_child(parent, name) != NULL) {
        set_error(error, error_size, "target already exists");
        return -1;
    }
    child = create_node(name, type, parent);
    if (child == NULL || add_child(parent, child) != 0) {
        free(child);
        set_error(error, error_size, "failed to create node");
        return -1;
    }
    (void)fs;
    return 0;
}

int fs_mkdir(FileSystem *fs, const char *path, char *error, size_t error_size)
{
    return create_child(fs, path, NODE_DIR, error, error_size);
}

int fs_create(FileSystem *fs, const char *path, char *error, size_t error_size)
{
    return create_child(fs, path, NODE_FILE, error, error_size);
}

static int free_block_count(const FileSystem *fs)
{
    int count = 0;
    int i;

    for (i = 0; i < fs->block_count; i++) {
        if (!fs->block_used[i]) {
            count++;
        }
    }
    return count;
}

static void release_file_blocks(FileSystem *fs, FsNode *file)
{
    int i;

    for (i = 0; i < file->block_count; i++) {
        int block = file->blocks[i];
        fs->block_used[block] = 0;
        free(fs->block_data[block]);
        fs->block_data[block] = NULL;
    }
    free(file->blocks);
    file->blocks = NULL;
    file->block_count = 0;
    file->size = 0;
}

static char *make_block_copy(const char *content, int offset, int length)
{
    char *copy = malloc((size_t)length + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, content + offset, (size_t)length);
    copy[length] = '\0';
    return copy;
}

/*
 * 覆盖写文件。
 * 修改前先检查空闲块并准备新内容块，确保空间不足或分配失败时旧文件内容不被破坏。
 */
int fs_write(FileSystem *fs, const char *path, const char *content, char *error,
             size_t error_size)
{
    FsNode *file;
    int content_len;
    int required_blocks;
    int available_blocks;
    int *new_blocks = NULL;
    char **new_data = NULL;
    int i;
    int next_block = 0;

    if (resolve_path(fs, path, &file, error, error_size) != 0) {
        return -1;
    }
    if (file->type != NODE_FILE) {
        set_error(error, error_size, "write target is not a file");
        return -1;
    }

    content_len = (int)strlen(content);
    required_blocks = content_len == 0 ? 0 : (content_len + fs->block_size - 1) / fs->block_size;
    available_blocks = free_block_count(fs) + file->block_count;
    if (available_blocks < required_blocks) {
        set_error(error, error_size, "not enough free blocks");
        return -1;
    }

    if (required_blocks > 0) {
        new_blocks = malloc(sizeof(int) * (size_t)required_blocks);
        new_data = calloc((size_t)required_blocks, sizeof(char *));
        if (new_blocks == NULL || new_data == NULL) {
            free(new_blocks);
            free(new_data);
            set_error(error, error_size, "failed to allocate write buffers");
            return -1;
        }
        for (i = 0; i < required_blocks; i++) {
            int offset = i * fs->block_size;
            int chunk_len = content_len - offset;
            if (chunk_len > fs->block_size) {
                chunk_len = fs->block_size;
            }
            new_data[i] = make_block_copy(content, offset, chunk_len);
            if (new_data[i] == NULL) {
                int j;
                for (j = 0; j < i; j++) {
                    free(new_data[j]);
                }
                free(new_data);
                free(new_blocks);
                set_error(error, error_size, "failed to allocate block data");
                return -1;
            }
        }
    }

    release_file_blocks(fs, file);
    for (i = 0; i < required_blocks; i++) {
        while (next_block < fs->block_count && fs->block_used[next_block]) {
            next_block++;
        }
        new_blocks[i] = next_block;
        fs->block_used[next_block] = 1;
        fs->block_data[next_block] = new_data[i];
        next_block++;
    }

    file->blocks = new_blocks;
    file->block_count = required_blocks;
    file->size = content_len;
    free(new_data);
    return 0;
}

int fs_read(FileSystem *fs, const char *path, char **content, char *error, size_t error_size)
{
    FsNode *file;
    char *buffer;
    int offset = 0;
    int i;

    if (resolve_path(fs, path, &file, error, error_size) != 0) {
        return -1;
    }
    if (file->type != NODE_FILE) {
        set_error(error, error_size, "read target is not a file");
        return -1;
    }

    buffer = malloc((size_t)file->size + 1);
    if (buffer == NULL) {
        set_error(error, error_size, "failed to allocate read buffer");
        return -1;
    }
    for (i = 0; i < file->block_count; i++) {
        const char *block = fs->block_data[file->blocks[i]];
        int chunk_len = (int)strlen(block);
        memcpy(buffer + offset, block, (size_t)chunk_len);
        offset += chunk_len;
    }
    buffer[offset] = '\0';
    *content = buffer;
    return 0;
}

int fs_ls(FileSystem *fs, const char *path, FsNode **dir, char *error, size_t error_size)
{
    FsNode *node;

    if (resolve_path(fs, path, &node, error, error_size) != 0) {
        return -1;
    }
    if (node->type != NODE_DIR) {
        set_error(error, error_size, "ls target is not a directory");
        return -1;
    }
    *dir = node;
    return 0;
}

int fs_delete(FileSystem *fs, const char *path, char *error, size_t error_size)
{
    FsNode *node;

    if (resolve_path(fs, path, &node, error, error_size) != 0) {
        return -1;
    }
    if (node->type != NODE_FILE) {
        set_error(error, error_size, "delete only supports files");
        return -1;
    }
    remove_child(node->parent, node);
    release_file_blocks(fs, node);
    free(node->children);
    free(node);
    return 0;
}

int fs_stat(FileSystem *fs, int *total_blocks, int *used_blocks, int *free_blocks, char *error,
            size_t error_size)
{
    int used = 0;
    int i;

    if (require_mounted(fs, error, error_size) != 0) {
        return -1;
    }
    for (i = 0; i < fs->block_count; i++) {
        if (fs->block_used[i]) {
            used++;
        }
    }
    *total_blocks = fs->block_count;
    *used_blocks = used;
    *free_blocks = fs->block_count - used;
    return 0;
}
