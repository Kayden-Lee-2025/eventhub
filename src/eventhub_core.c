#include "eventhub.h"
#include <string.h>

#if EVENTHUB_ENABLE_LOG
#define EVENTHUB_LOG(...) eventhub_port_log(__VA_ARGS__)
#else
#define EVENTHUB_LOG(...)
#endif

// 辅助函数：设置事件位
static inline void set_event_bit(uint32_t* mask, eventhub_event_type_t event_type) 
{
    if (event_type < EVENTHUB_MAX_EVENT_TYPES) 
    {
        uint32_t word_index = event_type / EVENT_MASK_BITS_PER_WORD;
        uint32_t bit_index = event_type % EVENT_MASK_BITS_PER_WORD;
        mask[word_index] |= (1U << bit_index);
    }
}

// 辅助函数：清除事件位
static inline void clear_event_bit(uint32_t* mask, eventhub_event_type_t event_type) 
{
    if (event_type < EVENTHUB_MAX_EVENT_TYPES) 
    {
        uint32_t word_index = event_type / EVENT_MASK_BITS_PER_WORD;
        uint32_t bit_index = event_type % EVENT_MASK_BITS_PER_WORD;
        mask[word_index] &= ~(1U << bit_index);
    }
}

// 辅助函数：检查事件位是否设置
static inline bool is_event_set(const uint32_t* mask, eventhub_event_type_t event_type) 
{
    if (event_type < EVENTHUB_MAX_EVENT_TYPES) 
    {
        uint32_t word_index = event_type / EVENT_MASK_BITS_PER_WORD;
        uint32_t bit_index = event_type % EVENT_MASK_BITS_PER_WORD;
        return (mask[word_index] & (1U << bit_index)) != 0;
    }
    return false;
}

// 辅助函数：检查位图是否为空
static inline bool is_mask_empty(const uint32_t* mask) 
{
    for (int i = 0; i < EVENT_MASK_WORDS; i++) 
    {
        if (mask[i] != 0) 
        {
            return false;
        }
    }
    return true;
}

bool eventhub_init(eventhub_t* hub) 
{
    if (hub == NULL) return false;
    memset(hub, 0, sizeof(eventhub_t));

    // 初始化互斥锁
    hub->priv.mutex = eventhub_port_mutex_init();
    if (NULL == hub->priv.mutex) 
    {
        EVENTHUB_LOG("eventhub: mutex init failed\n");
        return false;
    }

#if EVENTHUB_USING_RTOS
    // 初始化事件队列（存储eventhub_event_t类型）
    hub->priv.queue = eventhub_port_queue_init(sizeof(eventhub_event_t), EVENTHUB_QUEUE_SIZE);
    if (NULL == hub->priv.queue) 
    {
        EVENTHUB_LOG("eventhub: queue init failed\n");
        eventhub_port_mutex_destroy(hub->priv.mutex);
        return false;
    }
#endif

    EVENTHUB_LOG("eventhub: init success (RTOS=%d)\n", EVENTHUB_USING_RTOS);
    return true;
}

bool eventhub_subscribe(eventhub_t* hub, eventhub_event_type_t event_type,
                       eventhub_subscriber_cb cb, void* user_data) 
{
    if (hub == NULL || cb == NULL || event_type >= EVENTHUB_MAX_EVENT_TYPES) 
        return false;

    if (!eventhub_port_mutex_lock(hub->priv.mutex, 0))
    {
        EVENTHUB_LOG("eventhub: mutex lock failed\n");
        return false;
    }

    // 查找是否已存在该模块（相同回调和用户数据）
    for (uint16_t i = 0; i < EVENTHUB_MAX_MODULES; i++) 
    {
        if (hub->priv.module_subscribers[i].in_use &&
            hub->priv.module_subscribers[i].cb == cb &&
            hub->priv.module_subscribers[i].user_data == user_data) 
        {
            // 同一模块添加新事件类型
            set_event_bit(hub->priv.module_subscribers[i].event_mask, event_type);
            eventhub_port_mutex_unlock(hub->priv.mutex);
            EVENTHUB_LOG("eventhub: module subscribe event %d\n", event_type);
            return true;
        }
    }

    // 查找空位置注册新模块
    for (uint16_t i = 0; i < EVENTHUB_MAX_MODULES; i++) 
    {
        if (!hub->priv.module_subscribers[i].in_use) 
        {
            hub->priv.module_subscribers[i].cb = cb;
            hub->priv.module_subscribers[i].user_data = user_data;
            memset(hub->priv.module_subscribers[i].event_mask, 0, 
                   sizeof(hub->priv.module_subscribers[i].event_mask));
            set_event_bit(hub->priv.module_subscribers[i].event_mask, event_type);
            hub->priv.module_subscribers[i].in_use = true;
            eventhub_port_mutex_unlock(hub->priv.mutex);
            EVENTHUB_LOG("eventhub: new module subscribe event %d\n", event_type);
            return true;
        }
    }

    eventhub_port_mutex_unlock(hub->priv.mutex);
    EVENTHUB_LOG("eventhub: subscribe failed (max modules)\n");
    return false;
}

bool eventhub_unsubscribe(eventhub_t* hub, eventhub_event_type_t event_type,
                         eventhub_subscriber_cb cb) 
{
    if (hub == NULL || cb == NULL || event_type >= EVENTHUB_MAX_EVENT_TYPES) 
        return false;

    if (!eventhub_port_mutex_lock(hub->priv.mutex, 0))
    {
        EVENTHUB_LOG("eventhub: mutex lock failed\n");
        return false;
    }

    // 查找对应的模块订阅者
    for (uint16_t i = 0; i < EVENTHUB_MAX_MODULES; i++) 
    {
        if (hub->priv.module_subscribers[i].in_use &&
            hub->priv.module_subscribers[i].cb == cb) 
        {
            // 清除该事件类型的位
            clear_event_bit(hub->priv.module_subscribers[i].event_mask, event_type);
            
            // 如果该模块不再订阅任何事件，则完全取消订阅
            if (is_mask_empty(hub->priv.module_subscribers[i].event_mask)) 
            {
                hub->priv.module_subscribers[i].in_use = false;
            }
            
            eventhub_port_mutex_unlock(hub->priv.mutex);
            EVENTHUB_LOG("eventhub: unsubscribe event %d\n", event_type);
            return true;
        }
    }

    eventhub_port_mutex_unlock(hub->priv.mutex);
    EVENTHUB_LOG("eventhub: unsubscribe failed (not found)\n");
    return false;
}

bool eventhub_publish(eventhub_t* hub, const eventhub_event_t* event, uint32_t timeout) 
{
    if (hub == NULL || event == NULL) return false;

    // 填充时间戳
    eventhub_event_t event_with_ts = *event;
    event_with_ts.timestamp = eventhub_port_get_timestamp();

#if EVENTHUB_USING_RTOS
    // RTOS环境：事件放入队列，由eventhub_process任务处理
    bool ret = eventhub_port_queue_send(hub->priv.queue, &event_with_ts, timeout);
    if (ret) 
    {
        EVENTHUB_LOG("eventhub: publish event %d (queued)\n", event->type);
    } 
    else 
    {
        EVENTHUB_LOG("eventhub: publish event %d failed (queue full)\n", event->type);
    }
    return ret;
#else
    // 裸机环境：直接同步处理（在发布时调用回调）
    if (!eventhub_port_mutex_lock(hub->priv.mutex, timeout))
    {
        EVENTHUB_LOG("eventhub: mutex lock failed\n");
        return false;
    }
    
    // 处理所有订阅该事件类型的模块
    for (uint16_t i = 0; i < EVENTHUB_MAX_MODULES; i++) 
    {
        if (hub->priv.module_subscribers[i].in_use &&
            is_event_set(hub->priv.module_subscribers[i].event_mask, event->type)) 
        {
            // 调用模块的回调函数
            hub->priv.module_subscribers[i].cb(&event_with_ts, hub->priv.module_subscribers[i].user_data);
        }
    }
    
    eventhub_port_mutex_unlock(hub->priv.mutex);
    EVENTHUB_LOG("eventhub: publish event %d (sync)\n", event->type);
    return true;
#endif
}

void eventhub_process(eventhub_t* hub, uint32_t timeout) 
{
#if EVENTHUB_USING_RTOS
    // 参数验证
    if (hub == NULL) 
    {
        EVENTHUB_LOG("eventhub: invalid hub parameter\n");
        return;
    }

    // RTOS环境：从队列取事件并分发（通常在独立任务中运行）
    eventhub_event_t event;
    if (eventhub_port_queue_receive(hub->priv.queue, &event, timeout))
    {
        EVENTHUB_LOG("eventhub: received event %d from queue\n", event.type);
        
        // 检查互斥锁是否成功获取
        if (!eventhub_port_mutex_lock(hub->priv.mutex, 0))
        {
            EVENTHUB_LOG("eventhub: mutex lock failed during process\n");
            return;
        }
        
        // 遍历模块订阅者，调用回调
        for (uint16_t i = 0; i < EVENTHUB_MAX_MODULES; i++) 
        {
            if (hub->priv.module_subscribers[i].in_use &&
                is_event_set(hub->priv.module_subscribers[i].event_mask, event.type)) 
            {
                if (hub->priv.module_subscribers[i].cb != NULL) 
                {
                    hub->priv.module_subscribers[i].cb(&event, hub->priv.module_subscribers[i].user_data);
                }
            }
        }
        eventhub_port_mutex_unlock(hub->priv.mutex);
    }
#else
    // 裸机环境：空实现（发布时已同步处理）
    (void)hub;
    (void)timeout;
#endif
}

void eventhub_destroy(eventhub_t* hub) 
{
    if (hub == NULL) return;
    eventhub_port_mutex_destroy(hub->priv.mutex);
#if EVENTHUB_USING_RTOS
    eventhub_port_queue_destroy(hub->priv.queue);
#endif

    // 清理模块订阅者信息
    memset(&hub->priv.module_subscribers, 0, sizeof(hub->priv.module_subscribers));
}
