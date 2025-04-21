#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "mempool_port.h"

//===================================================================
//  通用实现
//===================================================================
// typedef uint32_t MEMPOOL_LOCK_TYPE; // 中断锁类型
// #define MEMPOOL_MALLOC(size)                malloc(size) // 内存分配
// #define MEMPOOL_FREE(ptr)                   free(ptr)    // 内存释放
// #define MEMPOOL_MEMALIGN(alignment, size)   aligned_alloc(alignment, size) // 内存对齐分配
// #define MEMPOOL_LOCK(lock)                  (lock = 1) // 中断锁定(伪代码)
// #define MEMPOOL_UNLOCK(lock)                (lock = 0) // 中断解锁(伪代码)
// #define MEMPOOL_ASSERT(expr)                assert(expr) // 断言(伪代码)
// #define MEMPOOL_MIN(a, b)                   ((a) < (b) ? (a) : (b))

// 配置宏
#define MEMPOOL_ALIGNMENT       64      // 内存对齐要求
#define MEMPOOL_MAX_BLOCKS      256     // 最大支持块数
#define MEMPOOL_MIN(a, b)       (((a) < (b)) ? (a) : (b))

// 根据块数量自动选择最优位图类型
#if MEMPOOL_MAX_BLOCKS <= 32
    #define BITMAP_TYPE uint32_t
    #define BITMAP_WORDS 1
    #define LOG2_MEMPOOL_BITMAP_EACH_NUM 5  // log2(32)
#elif MEMPOOL_MAX_BLOCKS <= 64
    #define BITMAP_TYPE uint64_t
    #define BITMAP_WORDS 1
    #define LOG2_MEMPOOL_BITMAP_EACH_NUM 6  // log2(64)
#elif MEMPOOL_MAX_BLOCKS <= 128
    #define BITMAP_TYPE uint64_t
    #define BITMAP_WORDS 2
    #define LOG2_MEMPOOL_BITMAP_EACH_NUM 6  // log2(64)
#elif MEMPOOL_MAX_BLOCKS <= 256
    #define BITMAP_TYPE uint64_t
    #define BITMAP_WORDS 4
    #define LOG2_MEMPOOL_BITMAP_EACH_NUM 6  // log2(64)
#else
    #error "MEMPOOL_MAX_BLOCKS exceeds maximum supported value"
#endif

#define MEMPOOL_BITMAP_EACH_NUM (sizeof(BITMAP_TYPE) * 8) // 位图类型大小(比特数)

typedef struct {
    uint8_t *memory_area;       // 内存区域基地址
    size_t block_size_unaligned;// 原始块大小(未对齐)
    size_t block_size;          // 每个块的大小(对齐后)
    size_t block_count;         // 实际块数量

    BITMAP_TYPE free_bitmap[BITMAP_WORDS];     // 空闲块位图
    BITMAP_TYPE hw_owned_bitmap[BITMAP_WORDS]; // 硬件占用标记

#ifdef MEMPOOL_LOCK_INIT
    MEMPOOL_LOCK_TYPE lock;
#endif
} mempool_t;

// 队列结构
typedef struct mempool_queue {
    struct mempool_queue *next;  // 用于构建优先级队列链表
    mempool_t *pool;
    uint16_t *block_indices;
    size_t *data_lengths;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    BITMAP_TYPE queue_bitmap[BITMAP_WORDS];
} mempool_queue_t;

// 内存池基础API
mempool_t *mempool_create(size_t data_size, size_t num_blocks);
void mempool_destroy(mempool_t *pool);

uint8_t *mempool_alloc(mempool_t *pool, bool for_hw);
void mempool_free(mempool_t *pool, uint8_t *ptr);

size_t mempool_block_size(mempool_t *pool);
size_t mempool_available(mempool_t *pool);
size_t mempool_used(mempool_t *pool);

// 队列API
mempool_queue_t *mempool_queue_create(mempool_t *pool, size_t capacity);
void mempool_queue_destroy(mempool_queue_t *queue);
int mempool_queue_enqueue(mempool_queue_t *queue, uint8_t *buffer);
uint8_t *mempool_queue_dequeue(mempool_queue_t *queue);
// 新增内部函数声明(仅在.c文件中使用，不需要在.h中声明)
// 新增公共接口
int mempool_queue_enqueue_with_length(mempool_queue_t *queue, uint8_t *buffer, size_t data_length);
uint8_t *mempool_queue_dequeue_with_length(mempool_queue_t *queue, size_t *data_length);
size_t mempool_queue_dequeue_batch_with_length(mempool_queue_t *queue, 
                                              uint8_t **buffers, 
                                              size_t *data_lengths,
                                              size_t max_count);

uint8_t *mempool_queue_peek(mempool_queue_t *queue);
size_t mempool_queue_dequeue_batch(mempool_queue_t *queue, uint8_t **buffers, size_t max_count);
size_t mempool_queue_count(mempool_queue_t *queue);
bool mempool_queue_is_empty(mempool_queue_t *queue);
bool mempool_queue_is_full(mempool_queue_t *queue);




#endif // MEMPOOL_H