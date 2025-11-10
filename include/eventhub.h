#ifndef EVENTHUB_H
#define EVENTHUB_H

#include <stdint.h>
#include <stdbool.h>
#include "eventhub_config.h"
#include "eventhub_port.h"

// 事件类型（用户需扩展具体类型，如EVENT_POWER_ON）
typedef uint32_t eventhub_event_type_t;

// 事件数据结构体
typedef struct 
{
    eventhub_event_type_t type;          // 事件类型
    eventhub_timestamp_t timestamp;      // 时间戳
    void* data;                          // 事件附加数据（可选）
    uint32_t data_len;                   // 附加数据长度
} eventhub_event_t;

// 订阅者回调函数原型
typedef void (*eventhub_subscriber_cb)(const eventhub_event_t* event, void* user_data);

// 模块订阅者结构体
typedef struct 
{
    eventhub_subscriber_cb cb;
    void* user_data;
    uint32_t event_mask[EVENT_MASK_WORDS];  // 位图数组表示该模块订阅的所有事件类型
    bool in_use;
} eventhub_module_subscriber_t;

// 事件中枢句柄（用户无需关心内部结构）
typedef struct 
{
    // 内部状态（订阅者列表、锁、队列等）
    struct 
    {
        eventhub_mutex_t* mutex;
#if EVENTHUB_USING_RTOS
        eventhub_queue_t* queue;
#endif
        // 模块订阅者列表
        eventhub_module_subscriber_t module_subscribers[EVENTHUB_MAX_MODULES];
    } priv;
} eventhub_t;

/**
 * 初始化事件中枢
 * @param hub 事件中枢实例
 * @return 成功返回true
 */
bool eventhub_init(eventhub_t* hub);

/**
 * 订阅事件（模块级订阅）
 * @param hub 事件中枢实例
 * @param event_type 事件类型
 * @param cb 回调函数
 * @param user_data 传给回调的用户数据
 * @return 成功返回true
 */
bool eventhub_subscribe(eventhub_t* hub, eventhub_event_type_t event_type,
                       eventhub_subscriber_cb cb, void* user_data);

/**
 * 取消订阅（模块级取消订阅）
 * @param hub 事件中枢实例
 * @param event_type 事件类型
 * @param cb 要取消的回调函数
 * @return 成功返回true
 */
bool eventhub_unsubscribe(eventhub_t* hub, eventhub_event_type_t event_type,
                         eventhub_subscriber_cb cb);

/**
 * 发布事件
 * @param hub 事件中枢实例
 * @param event 事件数据
 * @param timeout 超时时间（RTOS环境有效，0=非阻塞）
 * @return 成功返回true
 */
bool eventhub_publish(eventhub_t* hub, const eventhub_event_t* event, uint32_t timeout);

/**
 * 事件处理（裸机环境需在主循环调用，RTOS环境可作为任务）
 * @param hub 事件中枢实例
 * @param timeout 等待超时时间（RTOS用ticks，裸机忽略）
 */
void eventhub_process(eventhub_t* hub, uint32_t timeout);

/**
 * 销毁事件中枢
 * @param hub 事件中枢实例
 */
void eventhub_destroy(eventhub_t* hub);

#endif
