/*
 * 文件作用：集中输出文件系统命令结果、目录列表和块统计。
 * 设计原因：稳定字段能让测试脚本和实验报告直接复用命令输出。
 */
#include "filesystem.h"

#include <stdio.h>

void fs_print_ok(const char *message)
{
    printf("status: success\n");
    printf("%s\n", message);
}

void fs_print_error(const char *message)
{
    fprintf(stderr, "error: %s\n", message);
}

void fs_print_content(const char *content)
{
    printf("content: %s\n", content);
}

void fs_print_ls(const FsNode *dir)
{
    int i;

    printf("entries:\n");
    if (dir->child_count == 0) {
        printf("(empty)\n");
        return;
    }
    for (i = 0; i < dir->child_count; i++) {
        const FsNode *child = dir->children[i];
        printf("%s %s", child->name, child->type == NODE_DIR ? "dir" : "file");
        if (child->type == NODE_FILE) {
            printf(" size=%d blocks=", child->size);
            if (child->block_count == 0) {
                printf("-");
            } else {
                int j;
                for (j = 0; j < child->block_count; j++) {
                    if (j > 0) {
                        printf(",");
                    }
                    printf("%d", child->blocks[j]);
                }
            }
        }
        printf("\n");
    }
}

void fs_print_stat(int total_blocks, int used_blocks, int free_blocks)
{
    printf("total_blocks: %d\n", total_blocks);
    printf("used_blocks: %d\n", used_blocks);
    printf("free_blocks: %d\n", free_blocks);
}
