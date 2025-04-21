#include <stdint.h>
#include <string.h>
#include <mempool.h>
#include <mempool_log.h>
#include "net_device.h"

#define NET_DEVICE_USE_RX_ISR     0

static net_device_t *g_net_device_ptr = NULL;

// ======================================================================
// 硬件接口
// ======================================================================
#include <stdint.h>
#include <string.h>
#include <mempool.h>
#include "net_device.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define PYTHON_SERVER_IP   "127.0.0.1"
#define PYTHON_SERVER_PORT 1069

static int g_tcp_socket = -1;

// ======================================================================
// TCP通信接口
// ======================================================================

#include <pthread.h>

typedef struct {
    net_device_t *net_device;
    volatile bool running;
    pthread_t thread_id;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} receive_thread_t;

static receive_thread_t g_receive_thread;
static void *receive_thread_func(void *arg) {
    receive_thread_t *thread = (receive_thread_t *)arg;
    uint8_t *buffer = NULL;
    net_device_t *net_device = thread->net_device;

    if (!net_device || !net_device->mempool) {
        NET_LOGE("Invalid net_device or mempool");
        return NULL;
    }

    const size_t buffer_size = net_device->mempool->block_size; // 根据需要调整

    while (thread->running) {
        buffer = mempool_alloc(net_device->mempool, true);
        if (!buffer) {
            usleep(1000); // 10ms延迟
            continue;
        }

        ssize_t received = recv(g_tcp_socket, buffer, buffer_size, MSG_DONTWAIT);
        if (received > 0) {
            pthread_mutex_lock(&thread->mutex);
            NET_LOGD("Received %zd bytes from server", received);
            NET_HEX_DUMP(buffer, received);

            if (net_device->mempool_queue) {
                mempool_queue_enqueue_with_length(net_device->mempool_queue, buffer, received);
            }

            if (net_device->callback) {
                net_device->callback(NET_MSG_TYPE_RX_PACKET, net_device->userdata, buffer, received);
            }

            pthread_mutex_unlock(&thread->mutex);
        } 
        else if (received == 0) {
            NET_LOGE("Server disconnected");
            close(g_tcp_socket);
            g_tcp_socket = -1;
            mempool_free(net_device->mempool, buffer);
            break;
        }
        else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv error");
            }
            mempool_free(net_device->mempool, buffer);
            usleep(1000); // 1ms延迟避免忙等待
        }
    }

    return NULL;
}

int receive_thread_start(net_device_t *net_device) {
    g_receive_thread.net_device = net_device;
    g_receive_thread.running = true;

    pthread_mutex_init(&g_receive_thread.mutex, NULL);
    pthread_cond_init(&g_receive_thread.cond, NULL);

    if (pthread_create(&g_receive_thread.thread_id, NULL, 
                      receive_thread_func, &g_receive_thread) != 0) {
        perror("Failed to create receive thread");
        return -1;
    }

    return 0;
}

void receive_thread_stop() {
    g_receive_thread.running = false;
    pthread_join(g_receive_thread.thread_id, NULL);
    
    pthread_mutex_destroy(&g_receive_thread.mutex);
    pthread_cond_destroy(&g_receive_thread.cond);
}

static int tcp_connect() {
    if (g_tcp_socket >= 0) {
        return 0; // 已连接
    }

    g_tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_tcp_socket < 0) {
        perror("socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PYTHON_SERVER_PORT),
        .sin_addr.s_addr = inet_addr(PYTHON_SERVER_IP)
    };

    if (connect(g_tcp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        close(g_tcp_socket);
        g_tcp_socket = -1;
        return -1;
    }

    // 现在只需要启动接收线程
    static bool thread_started = false;
    
    if (!thread_started) {
        if (receive_thread_start(g_net_device_ptr) == 0) {
            thread_started = true;
        }
    }

    NET_LOGD("Connected to Python server at %s:%d", 
           PYTHON_SERVER_IP, PYTHON_SERVER_PORT);
    return 0;
}
// ======================================================================
// 修改后的硬件模拟接口
// ======================================================================

int hw_simulate_send(uint8_t *data, size_t length) {
    if (tcp_connect() < 0) {
        return -1;
    }

    // 打印调试信息
    NET_LOGD("Sending %zu bytes to server", length);
    NET_HEX_DUMP(data, length);

    // 通过TCP发送数据
    ssize_t sent = send(g_tcp_socket, data, length, 0);
    if (sent < 0) {
        perror("send failed");
        return -1;
    }

    // 模拟发送完成中断
    // hw_simulate_send_isr(&g_net_device, data, length);
    return 0;
}

static void hw_simulate_send_isr(net_device_t *net_device, uint8_t *buffer, size_t length) {
    if (net_device->callback) {
        net_device->callback(NET_MSG_TYPE_TX_PACKET, net_device->userdata, buffer, length);
    }
    mempool_free(net_device->mempool, buffer);
}

static void hw_simulate_receive_isr(net_device_t *net_device) {
    // 现在只需要启动接收线程
    static bool thread_started = false;
    
    if (!thread_started) {
        if (receive_thread_start(net_device) == 0) {
            thread_started = true;
        }
    }
}

uint32_t net_get_time_ms(void) {
    return MEMPOOL_CURRENT_TIME_MS();
}

// ======================================================================
// 网络中间适配层
// ======================================================================
// 发送数据，发送完成后，需要释放网络内存池的buffer
int net_send(net_device_t *dev, uint8_t *data, size_t length)
{
    uint8_t *buffer = mempool_alloc(dev->mempool, true);
    if(!buffer) {
        NET_LOGE("Failed to allocate buffer for sending");
        return -1;
    }
    
    memcpy(buffer, data, length); 

    hw_simulate_send(data, length);
    return 0;
}

// 接收到数据后，搬运到网络的内存池内，然后发送给网络（里面会校验是否是有效的以太报文，可能发送多次）
// 给网络协议栈发送消息通知，有新的数据进来（只通知一次，但是这次通知可能是多条报文进入）
// 接收数据
int net_receive_pool(net_device_t *dev, uint8_t *data, size_t length) {
    // 模拟接收数据
    uint8_t *buffer = NULL;
    size_t data_length = 0;

    if (dev->mempool_queue) {
        buffer = mempool_queue_dequeue_with_length(dev->mempool_queue, &data_length);
        if(buffer == NULL) {
            return -1;
        }
        NET_LOGD("Received %zu bytes from pool", data_length);
        memcpy(data, buffer, length < data_length ? length : data_length);
        mempool_free(dev->mempool, buffer);
        return length < data_length ? length : data_length;
    }

    return 0;
}

// 直接获取内存地址，零拷贝
uint8_t *net_receive_zerocpy(net_device_t *dev) {
    // 直接从内存池中获取数据
    if (dev->mempool_queue) {
        return mempool_queue_dequeue(dev->mempool_queue);
    }

    return NULL;
}

uint8_t *net_receive_zerocpy_with_length(net_device_t *dev, size_t *length) {
    // 直接从内存池中获取数据
    if (dev->mempool_queue) {
        return mempool_queue_dequeue_with_length(dev->mempool_queue, length);
    }

    return NULL;
}

// 检查是否有数据到达
// 这里的检查是为了避免在没有数据到达的情况下，调用net_receive_pool函数
int net_check_packet_input(net_device_t *dev) {
    if (dev->mempool_queue) {
        if(mempool_queue_count(dev->mempool_queue) > 0) {
            return 1;
        }
    }

    return 0;
}

uint8_t *net_packet_alloc(net_device_t *dev, size_t length)
{
    if(length > mempool_block_size(dev->mempool)) {
        NET_LOGE("Requested length exceeds block size");
        return NULL;
    }

    return mempool_alloc(dev->mempool, false);
}

void net_packet_free(net_device_t *dev, uint8_t *buffer) {
    if (dev->mempool) {
        mempool_free(dev->mempool, buffer);
    }
}

int net_init(net_device_t *dev) {
    DEBUG_PRINT("Initializing network device");

    g_net_device_ptr = dev;

    // 初始化内存池
    dev->mempool = mempool_create(1600, 10);
    if (!dev->mempool) {
        NET_LOGE("Failed to create memory pool");
        return -1;
    }

    // 创建队列
    dev->mempool_queue = mempool_queue_create(dev->mempool, 10);
    if (!dev->mempool_queue) {
        NET_LOGE("Failed to create memory pool queue");
        mempool_destroy(dev->mempool);
        return -1;
    }

    // 驱动初始化
    tcp_connect();

    return 0;
}



// tftp使用
