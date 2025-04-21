#ifndef NET_DEVICE_H
#define NET_DEVICE_H

#include <stdint.h>
#include <mempool.h>

#define NET_LOG_LEVEL_DEBUG     0
#define NET_LOG_LEVEL_INFO      1
#define NET_LOG_LEVEL_WARNING   2
#define NET_LOG_LEVEL_ERROR     3

#ifndef NET_LOG_LEVEL
#define NET_LOG_LEVEL NET_LOG_LEVEL_DEBUG  // 默认日志级别
#endif

void net_base_log(uint8_t level, const char* file, int line, const char* fmt, ...);
void net_base_hex(const unsigned char *data, size_t length);

// 各级别日志宏
#if NET_LOG_LEVEL <= NET_LOG_LEVEL_DEBUG
#define NET_LOGD(fmt, ...) net_base_log(NET_LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define NET_LOGD(fmt, ...) ((void)0)
#endif

#if NET_LOG_LEVEL <= NET_LOG_LEVEL_INFO
#define NET_LOGI(fmt, ...)  net_base_log(NET_LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define NET_LOGI(fmt, ...)  ((void)0)
#endif

#if NET_LOG_LEVEL <= NET_LOG_LEVEL_WARNING
#define NET_LOGW(fmt, ...) net_base_log(NET_LOG_LEVEL_WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define NET_LOGW(fmt, ...) ((void)0)
#endif

#define NET_HEX_DUMP(data, length)  net_base_hex((const unsigned char *)data, (size_t)length)

// ERROR总是打印
#define NET_LOGE(fmt, ...) net_base_log(NET_LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

// 创建任务

#define NET_USE_ASYNC_TASK    1


void *net_create_task(void (*start_routine)(void *), void *arg);

int net_task_start(void *task);

int net_task_delete(void *task);

void *net_create_sem(void);
int net_sem_wait(void *sem);
int net_sem_post(void *sem);
int net_sem_destroy(void *sem);

#define NET_MTU_MAX 1500 // 假设最大传输单元为1500字节



typedef enum {
    NET_MSG_TYPE_NONE = 0,
    NET_MSG_TYPE_RX_PACKET,
    NET_MSG_TYPE_TX_PACKET,
    NET_MSG_TYPE_MAX
}NET_MSG_TYPE;

typedef void (*net_dev_callback_t)(NET_MSG_TYPE msg_type, void *userdata, uint8_t *data, size_t length);

typedef struct net_device_ops
{
    void (*tx_callback)(uint8_t *buffer, size_t length); // 发送完成
    void (*rx_callback)(uint8_t *buffer, size_t length); // 接收完成
} net_device_ops_t;

typedef struct 
{
    net_device_ops_t ops;   // 设备操作函数指针
    void *userdata;         // 用户数据指针
    mempool_t *mempool;     // 内存池指针
    mempool_queue_t *mempool_queue; // 内存池队列指针

    net_dev_callback_t callback; // 回调函数

} net_device_t;

uint32_t net_get_time_ms(void);

// 发送以太网数据
int net_send(net_device_t *dev, uint8_t *data, size_t length);
// 接收以太网数据
int net_receive_pool(net_device_t *dev, uint8_t *data, size_t length);

// 零拷贝接收数据
uint8_t *net_receive_zerocpy(net_device_t *dev);
uint8_t *net_receive_zerocpy_with_length(net_device_t *dev, size_t *length);

// 释放接收的数据
uint8_t *net_packet_alloc(net_device_t *dev, size_t length);
void net_packet_free(net_device_t *dev, uint8_t *buffer);

// 初始化网络设备
int net_init(net_device_t *dev);



#endif