#ifndef EVENTHUB_CONFIG_H
#define EVENTHUB_CONFIG_H

// 环境选择：0=裸机，1=RTOS
#define EVENTHUB_USING_RTOS 0

// 最大事件类型（根据业务扩展）
#define EVENTHUB_MAX_EVENT_TYPES 10

// 最大订阅者数量（根据内存调整）
#define EVENTHUB_MAX_SUBSCRIBERS 8

// 事件队列大小（仅RTOS环境有效）
#define EVENTHUB_QUEUE_SIZE 16

// 是否启用事件日志（调试用）
#define EVENTHUB_ENABLE_LOG 0

#endif
