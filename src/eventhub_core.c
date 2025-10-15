#include "eventhub.h"
#include <string.h>

#if EVENTHUB_ENABLE_LOG
#define EVENTHUB_LOG(...) eventhub_port_log(__VA_ARGS__)
#else
#define EVENTHUB_LOG(...)
#endif

bool eventhub_init(eventhub_t* hub) 
{
    if (hub == NULL) return false;
    memset(hub, 0, sizeof(eventhub_t));

    // 初始化互斥锁
    if (!eventhub_port_mutex_init(&hub->priv.mutex)) 
    {
        EVENTHUB_LOG("eventhub: mutex init failed\n");
        return false;
    }

#if EVENTHUB_USING_RTOS
    // 初始化事件队列（存储eventhub_event_t类型）
    if (!eventhub_port_queue_init(&hub->priv.queue, sizeof(eventhub_event_t), EVENTHUB_QUEUE_SIZE)) 
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
    if (hub == NULL || cb == NULL) return false;

    if (!eventhub_port_mutex_lock(hub->priv.mutex, 0))
    {
        EVENTHUB_LOG("eventhub: mutex lock failed\n");
        return false;
    }

    // 查找空位置注册订阅者
    for (uint8_t i = 0; i < EVENTHUB_MAX_SUBSCRIBERS; i++) 
    {
        if (!hub->priv.subscribers[i].in_use) 
        {
            hub->priv.subscribers[i].event_type = event_type;
            hub->priv.subscribers[i].cb = cb;
            hub->priv.subscribers[i].user_data = user_data;
            hub->priv.subscribers[i].in_use = true;
            eventhub_port_mutex_unlock(hub->priv.mutex);
            EVENTHUB_LOG("eventhub: subscribe event %d\n", event_type);
            return true;
        }
    }

    eventhub_port_mutex_unlock(hub->priv.mutex);
    EVENTHUB_LOG("eventhub: subscribe failed (max subscribers)\n");
    return false;
}

bool eventhub_unsubscribe(eventhub_t* hub, eventhub_event_type_t event_type,
                         eventhub_subscriber_cb cb) 
{
    if (hub == NULL || cb == NULL) return false;

    if (!eventhub_port_mutex_lock(hub->priv.mutex, 0))
    {
        EVENTHUB_LOG("eventhub: mutex lock failed\n");
        return false;
    }

    for (uint8_t i = 0; i < EVENTHUB_MAX_SUBSCRIBERS; i++) 
    {
        if (hub->priv.subscribers[i].in_use &&
            hub->priv.subscribers[i].event_type == event_type &&
            hub->priv.subscribers[i].cb == cb) 
        {
            hub->priv.subscribers[i].in_use = false;
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
    
    // 处理所有订阅该事件类型的订阅者
    for (uint8_t i = 0; i < EVENTHUB_MAX_SUBSCRIBERS; i++) 
    {
        if (hub->priv.subscribers[i].in_use &&
            hub->priv.subscribers[i].event_type == event->type) 
        {
            // 调用订阅者的回调函数
            hub->priv.subscribers[i].cb(&event_with_ts, hub->priv.subscribers[i].user_data);
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
        
        // 遍历订阅者，调用回调
        for (uint8_t i = 0; i < EVENTHUB_MAX_SUBSCRIBERS; i++) 
        {
            if (hub->priv.subscribers[i].in_use &&
                hub->priv.subscribers[i].event_type == event.type) 
            {
                if (hub->priv.subscribers[i].cb != NULL) 
                {
                    hub->priv.subscribers[i].cb(&event, hub->priv.subscribers[i].user_data);
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

    // 清理订阅者信息（可选的安全措施）
    memset(&hub->priv.subscribers, 0, sizeof(hub->priv.subscribers));
}
