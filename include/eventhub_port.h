#ifndef EVENTHUB_PORT_H
#define EVENTHUB_PORT_H

#include <stdint.h>
#include <stdbool.h>
#include "eventhub_config.h"

// 时间戳类型（毫秒级）
typedef uint32_t eventhub_timestamp_t;

// 互斥锁类型（统一使用值类型）
#if EVENTHUB_USING_RTOS
typedef void eventhub_mutex_t;
#else
typedef struct { int dummy; } eventhub_mutex_t;
#endif

// 事件队列类型（仅RTOS环境有效）
#if EVENTHUB_USING_RTOS
typedef void eventhub_queue_t;
#endif

/**
 * 初始化互斥锁
 * @return 成功返回互斥锁对象，失败返回NULL
 */
eventhub_mutex_t* eventhub_port_mutex_init(void);

/**
 * 上锁
 * @param mutex 互斥锁对象
 * @param timeout 超时时间（RTOS用ticks，裸机忽略）
 * @return 成功返回true
 */
bool eventhub_port_mutex_lock(eventhub_mutex_t* mutex, uint32_t timeout);

/**
 * 解锁
 * @param mutex 互斥锁对象
 */
void eventhub_port_mutex_unlock(eventhub_mutex_t* mutex);

/**
 * 销毁互斥锁
 * @param mutex 互斥锁对象
 */
void eventhub_port_mutex_destroy(eventhub_mutex_t* mutex);

/**
 * 获取当前时间戳（毫秒）
 * @return 当前时间戳
 */
eventhub_timestamp_t eventhub_port_get_timestamp(void);

#if EVENTHUB_USING_RTOS
/**
 * 初始化事件队列（RTOS环境）
 * @param item_size 队列项大小
 * @param queue_len 队列长度
 * @return 成功返回消息队列对象，失败返回NULL
 */
eventhub_queue_t* eventhub_port_queue_init(uint32_t item_size, uint32_t queue_len);

/**
 * 向队列发送数据（RTOS环境）
 * @param queue 队列对象
 * @param data 要发送的数据指针
 * @param timeout 超时时间
 * @return 成功返回true
 */
bool eventhub_port_queue_send(eventhub_queue_t* queue, const void* data, uint32_t timeout);

/**
 * 从队列接收数据（RTOS环境）
 * @param queue 队列对象
 * @param data 接收数据的缓冲区指针
 * @param timeout 超时时间
 * @return 成功返回true
 */
bool eventhub_port_queue_receive(eventhub_queue_t* queue, void* data, uint32_t timeout);

/**
 * 销毁队列（RTOS环境）
 * @param queue 队列对象
 */
void eventhub_port_queue_destroy(eventhub_queue_t* queue);
#endif

#if EVENTHUB_ENABLE_LOG
/**
 * 日志输出函数（用户实现，如UART打印）
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void eventhub_port_log(const char* format, ...);
#endif

#endif
