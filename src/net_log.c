#include <stddef.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <net_device.h>
#include <ctype.h> // ??
#include <pthread.h>
#include <semaphore.h>

#define NET_MALLOC(size)    malloc(size)
#define NET_FREE(ptr)       free(ptr)

// ANSI颜色定义（整行着色）
#define COLOR_DEBUG   "\033[0;36m"  // 青色
#define COLOR_INFO    "\033[0;32m"  // 绿色
#define COLOR_WARNING "\033[0;33m"  // 黄色
#define COLOR_ERROR   "\033[1;31m"  // 红色（加粗）
#define COLOR_RESET   "\033[0m"     // 重置

typedef struct net_task
{
    pthread_t thread_id;
    void (*start_routine)(void *);
    void *arg;
    int is_running;
    int is_joined;
}net_task_t;

typedef struct net_sem {
    void *userdata; // 用户数据
    sem_t semaphore; // POSIX信号量
}net_sem_t;

// 获取当前时间字符串（线程安全版本）
static inline const char* get_timestamp() {
    static __thread char buffer[32];
    time_t now = time(NULL);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return buffer;
}

void net_base_hex(const unsigned char *data, size_t length) {
    const int BYTES_PER_LINE = 16;
    
    for (size_t offset = 0; offset < length; offset += BYTES_PER_LINE) {
        // 打印偏移量
        printf("%08zx  ", offset);
        
        // 打印十六进制数据
        for (int i = 0; i < BYTES_PER_LINE; i++) {
            if (offset + i < length) {
                printf("%02x ", data[offset + i]);
            } else {
                printf("   "); // 对齐填充
            }
            
            // 在8字节后添加额外空格
            if (i == 7) {
                printf(" ");
            }
        }
        
        printf(" ");
        
        // 打印ASCII字符
        for (int i = 0; i < BYTES_PER_LINE; i++) {
            if (offset + i < length) {
                unsigned char c = data[offset + i];
                printf("%c", isprint(c) ? c : '.');
            } else {
                printf(" "); // 对齐填充
            }
        }
        
        printf("\n");
    }
}

// 基础日志函数（整行着色）
void net_base_log(uint8_t level, const char* file, int line, const char* fmt, ...) {
    const char* color = COLOR_RESET;
    const char* prefix = "";
    
    switch(level) {
        case NET_LOG_LEVEL_DEBUG:   color = COLOR_DEBUG;   prefix = "[DEBUG]  "; break;
        case NET_LOG_LEVEL_INFO:    color = COLOR_INFO;    prefix = "[INFO]   "; break;
        case NET_LOG_LEVEL_WARNING: color = COLOR_WARNING; prefix = "[WARNING]"; break;
        case NET_LOG_LEVEL_ERROR:   color = COLOR_ERROR;   prefix = "[ERROR]  "; break;
    }
    
    // 打印带颜色的日志头
    // fprintf(stderr, "%s%s %s:%d ", color, prefix, file, line);
    fprintf(stderr, "%s%s  ", color, prefix);
    
    // 打印带颜色的日志内容
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    // 重置颜色并换行
    fprintf(stderr, "%s\n", COLOR_RESET);
}

static void *net_task_adapter(void *arg) {
    net_task_t *task = (net_task_t *)arg;
    task->start_routine(task->arg);
    return NULL;
}

void *net_create_task(void (*start_routine)(void *), void *arg)
{
    net_task_t *task = (net_task_t *)NET_MALLOC(sizeof(net_task_t));
    if (task == NULL) {
        NET_LOGE("Failed to allocate memory for task");
        return NULL;
    }
    task->start_routine = start_routine;
    task->arg = arg;
    task->is_running = 0;
    task->is_joined = 0;
    task->thread_id = 0;
    return (void *)task;
}

int net_task_start(void *task)
{
    if (task == NULL) {
        NET_LOGE("Task is NULL");
        return -1;
    }

    if (pthread_create(&((net_task_t *)task)->thread_id, NULL, net_task_adapter, (net_task_t *)task) != 0) {
        NET_LOGE("Failed to create task");
        NET_FREE(task);
        return -1;
    }
    
    ((net_task_t *)task)->is_running = 1;
    ((net_task_t *)task)->is_joined = 0;

    return 0;
}

int net_task_delete(void *task)
{
    if (task == NULL) {
        NET_LOGE("Task is NULL");
        return -1;
    }

    if (((net_task_t *)task)->is_running) {
        pthread_cancel(((net_task_t *)task)->thread_id);
        ((net_task_t *)task)->is_running = 0;
    }

    if (!((net_task_t *)task)->is_joined) {
        pthread_join(((net_task_t *)task)->thread_id, NULL);
        ((net_task_t *)task)->is_joined = 1;
    }

    NET_FREE(task);
    return 0;
}


void *net_create_sem(void)
{
    net_sem_t *sem = (net_sem_t *)NET_MALLOC(sizeof(net_sem_t));
    if (sem == NULL) {
        NET_LOGE("Failed to allocate memory for semaphore");
        return NULL;
    }

    if (sem_init(&sem->semaphore, 0, 1) != 0) {
        NET_LOGE("Failed to initialize semaphore");
        NET_FREE(sem);
        return NULL;
    }

    sem->userdata = NULL;

    return sem;
}

int net_sem_wait(void *sem)
{
    if (sem == NULL) {
        NET_LOGE("Semaphore is NULL");
        return -1;
    }

    return sem_wait(&((net_sem_t *)sem)->semaphore);
}

int net_sem_post(void *sem)
{
    if (sem == NULL) {
        NET_LOGE("Semaphore is NULL");
        return -1;
    }

    return sem_post(&((net_sem_t *)sem)->semaphore);
}

int net_sem_destroy(void *sem)
{
    if (sem == NULL) {
        NET_LOGE("Semaphore is NULL");
        return -1;
    }

    sem_destroy(&((net_sem_t *)sem)->semaphore);
    NET_FREE(sem);

    return 0;
}
