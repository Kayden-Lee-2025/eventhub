#ifndef EVENTHUB_CONFIG_H
#define EVENTHUB_CONFIG_H

// 环境选择：0=裸机，1=RTOS
#define EVENTHUB_USING_RTOS 1

// 最大模块订阅者数量（根据实际功能模块数量调整）
#define EVENTHUB_MAX_MODULES 64

// 事件队列大小（仅RTOS环境有效）
#define EVENTHUB_QUEUE_SIZE 16

// 是否启用事件日志（调试用）
#define EVENTHUB_ENABLE_LOG 0

// 支持的最大事件类型数量
#define EVENTHUB_MAX_EVENT_TYPES 1024

// 位图配置：每个位图字包含的位数
#define EVENT_MASK_BITS_PER_WORD 32

// 计算需要的位图字数量以支持所有事件类型
#define EVENT_MASK_WORDS ((EVENTHUB_MAX_EVENT_TYPES + EVENT_MASK_BITS_PER_WORD - 1) / EVENT_MASK_BITS_PER_WORD)

#endif
