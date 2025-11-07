#include "eventhub_port.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

// FreeRTOS互斥锁实现
eventhub_mutex_t* eventhub_port_mutex_init(void)
{
    return xSemaphoreCreateMutex();
}

bool eventhub_port_mutex_lock(eventhub_mutex_t* mutex, uint32_t timeout) 
{
    if (mutex == NULL) return false;
    
    return xSemaphoreTake(mutex, timeout) == pdTRUE;
}

void eventhub_port_mutex_unlock(eventhub_mutex_t* mutex) 
{
    if (mutex == NULL) return;
    
    xSemaphoreGive(mutex);
}

void eventhub_port_mutex_destroy(eventhub_mutex_t* mutex) 
{
    if (mutex == NULL) return;
    
    vSemaphoreDelete(mutex);
}

// 时间戳：FreeRTOS系统时间（ms）
eventhub_timestamp_t eventhub_port_get_timestamp(void) 
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

// FreeRTOS队列实现
eventhub_queue_t* eventhub_port_queue_init(uint32_t item_size, uint32_t queue_len)
{
    return xQueueCreate(queue_len, item_size);
}

bool eventhub_port_queue_send(eventhub_queue_t* queue, const void* data, uint32_t timeout) 
{
    if (queue == NULL || data == NULL) return false;
    
    return xQueueSend(queue, data, timeout) == pdPASS;
}

bool eventhub_port_queue_receive(eventhub_queue_t* queue, void* data, uint32_t timeout) 
{
    if (queue == NULL || data == NULL) return false;
    
    return xQueueReceive(queue, data, timeout) == pdPASS;
}

void eventhub_port_queue_destroy(eventhub_queue_t* queue) 
{
    if (queue == NULL) return;
    
    vQueueDelete(queue);
}

#if EVENTHUB_ENABLE_LOG
// 日志输出：通过FreeRTOS的printf（需重定向）
#include <stdio.h>
#include <stdarg.h>

void eventhub_port_log(const char* format, ...) 
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
#endif
