/*
 * 文件作用：提供同步模块的线程事件输出辅助函数。
 * 设计原因：三个问题都需要展示线程动作，统一格式能让运行记录和验证脚本更容易阅读。
 */
#include "sync.h"

#include <stdio.h>

void log_event(const char *problem, int id, const char *action, int value)
{
    printf("%s thread=%d action=%s value=%d\n", problem, id, action, value);
}
