#include "net_device.h"
#include <stdio.h>
#include <string.h>

// 测试用的回调函数
void test_callback(NET_MSG_TYPE msg_type, void *userdata, uint8_t *data, size_t length) {
    NET_LOGI("Callback received message type: %d", msg_type);
    NET_LOGI("User data: %p", userdata);
    
    if (data && length > 0) {
        NET_LOGI("Data length: %zu", length);
        NET_HEX_DUMP(data, length > 16 ? 16 : length); // 只打印前16字节
    }
}

// 测试用的设备操作函数
void test_tx_callback(uint8_t *buffer, size_t length) {
    NET_LOGI("TX complete, length: %zu", length);
}

void test_rx_callback(uint8_t *buffer, size_t length) {
    NET_LOGI("RX complete, length: %zu", length);
}

// 测试任务函数
void test_task(void *arg) {
    net_device_t *dev = (net_device_t *)arg;
    NET_LOGI("Test task started");
    
    // 模拟接收数据
    uint8_t rx_data[] = {0x11, 0x22, 0x33, 0x44, 0x55};
    net_receive_pool(dev, rx_data, sizeof(rx_data));
    
    NET_LOGI("Test task completed");
}

int main() {
    NET_LOGI("Starting network device test");
    
    // 初始化网络设备
    net_device_t dev = {0};
    dev.ops.tx_callback = test_tx_callback;
    dev.ops.rx_callback = test_rx_callback;
    dev.callback = test_callback;
    dev.userdata = (void *)0x12345678; // 测试用用户数据
    
    if (net_init(&dev) != 0) {
        NET_LOGE("Failed to initialize network device");
        return -1;
    }
    
    // 测试发送数据
    uint8_t tx_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    NET_LOGI("Sending test data");
    if (net_send(&dev, tx_data, sizeof(tx_data)) != 0) {
        NET_LOGE("Failed to send data");
    }
    
    // 测试零拷贝接收
    NET_LOGI("Testing zero-copy receive");
    size_t length = 0;
    uint8_t *rx_data = net_receive_zerocpy_with_length(&dev, &length);
    if (rx_data) {
        NET_LOGI("Received %zu bytes via zero-copy", length);
        NET_HEX_DUMP(rx_data, length > 16 ? 16 : length);
        net_packet_free(&dev, rx_data);
    }
    
    // 测试异步任务
#if NET_USE_ASYNC_TASK
    NET_LOGI("Testing async task");
    void *task = net_create_task(test_task, &dev);
    if (task) {
        net_task_start(task);
        // 这里应该等待任务完成，简化示例中直接继续
        net_task_delete(task);
    }
#endif
    
    // 测试内存池分配
    NET_LOGI("Testing memory pool");
    uint8_t *alloc_data = net_packet_alloc(&dev, 128);
    if (alloc_data) {
        NET_LOGI("Allocated 128 bytes from pool");
        memset(alloc_data, 0xAA, 128);
        NET_HEX_DUMP(alloc_data, 16);
        net_packet_free(&dev, alloc_data);
    }
    
    NET_LOGI("Network device test completed");
    return 0;
}