NetFlex Module - 跨平台以太网报文传输框架​
​模块定位​
NetFlex 是一个硬件无关、OS无关的以太网报文传输抽象层，通过统一接口实现：

✅ ​原始以太网帧收发​（支持IEEE 802.3标准）
✅ ​多传输后端热切换​（TCP/IP模拟/真实硬件）
✅ ​无缝适配裸机/RTOS/Linux环境
​核心设计​
1. ​命名一致性实现​
c
复制
// 所有API均以`netflex_`前缀开头（如头文件定义）
#define NETFLEX_MODE_ETH    0x01  // 真实以太网硬件
#define NETFLEX_MODE_TCPIP  0x02  // TCP/IP模拟链路
2. ​以太网报文处理​
​帧结构支持​
字段	API示例	说明
MAC地址	netflex_set_mac()	配置源MAC
VLAN标签	netflex_add_vlan_tag()	插入802.1Q标签
载荷类型	netflex_set_ethertype()	设置0x0800(IP)等类型
c
复制
// 快速发送IPv4报文示例
uint8_t frame[64];
netflex_build_eth_header(frame, dest_mac, NETFLEX_ETHERTYPE_IP);
netflex_send(frame, sizeof(frame));  // 自动填充CRC
3. ​跨平台适配​
​后端抽象架构​
mermaid
复制
graph TD
    A[应用层] --> B(NetFlex统一API)
    B --> C{传输后端}
    C --> D[Linux Socket]
    C --> E[STM32 HAL+ETH]
    C --> F[FreeRTOS+LWIP]
​平台初始化示例​
c
复制
// Linux环境（需root权限）
netflex_init(NETFLEX_MODE_ETH, "eth0"); 

// STM32Cube环境
netflex_init(NETFLEX_MODE_ETH, &heth); 

// 裸机模拟测试
netflex_init(NETFLEX_MODE_TCPIP, "127.0.0.1:8888");
​关键API​
函数	作用	线程安全
netflex_rx_register_cb()	注册异步接收回调	是（锁可选）
netflex_get_link_status()	获取物理链路状态	依赖硬件实现
netflex_set_promiscuous()	启用混杂模式（抓包）	需后端支持
​性能优化​
​零拷贝模式​

c
复制
// 直接获取接收缓冲区（避免memcpy）
uint8_t *pkt = netflex_rx_borrow_buf(); 
process_packet(pkt);  // 处理数据
netflex_rx_release_buf();  // 释放缓冲区
​内存池预分配​
通过netflex_config_mempool()预分配帧内存，避免实时分配。

​扩展应用​
1. ​协议栈集成​
c
复制
// 与lwIP协议栈对接示例
err_t netflex_if_output(struct netif *netif, struct pbuf *p) {
    netflex_send(p->payload, p->len);  // 直接发送pbuf
    return ERR_OK;
}
2. ​硬件加速支持​
预留DMA接口：

c
复制
// 注册DMA传输回调（如STM32 ETH）
netflex_reg_dma_cb(&dma_tx_complete, &dma_rx_ready);
