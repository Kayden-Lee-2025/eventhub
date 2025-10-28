#include "eventhub_port.h"
#include "stm32f1xx_hal.h" // 以STM32为例，用户需替换为自己的硬件库

// 裸机互斥锁（无需实际锁，因为无多任务）
bool eventhub_port_mutex_init(eventhub_mutex_t* mutex) 
{
    if (mutex == NULL) return false;
    
    // 裸机环境下mutex是一个结构体，这里可以初始化结构体成员
    mutex->dummy = 0;
    return true;
}

bool eventhub_port_mutex_lock(eventhub_mutex_t* mutex, uint32_t timeout) 
{
    (void)timeout; // 裸机环境下忽略超时参数
    (void)mutex;   // 裸机环境下无需实际操作
    
    // 裸机环境下无需实际上锁操作
    return true;
}

void eventhub_port_mutex_unlock(eventhub_mutex_t* mutex) 
{
    (void)mutex;   // 裸机环境下无需实际操作
    
    // 裸机环境下无需实际解锁操作
}

void eventhub_port_mutex_destroy(eventhub_mutex_t* mutex) 
{
    (void)mutex;   // 裸机环境下无需实际操作
    
    // 裸机环境下无需实际销毁操作
}

// 时间戳：使用SysTick定时器（假设已初始化，1ms中断）
static volatile uint32_t sys_tick_ms = 0;

void SysTick_Handler(void) 
{
    sys_tick_ms++;
}

eventhub_timestamp_t eventhub_port_get_timestamp(void) 
{
    return sys_tick_ms;
}

#if EVENTHUB_ENABLE_LOG
// 日志输出：通过UART
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// 假设已定义UART句柄，用户需根据实际硬件修改
extern UART_HandleTypeDef huart1;

void eventhub_port_log(const char* format, ...) 
{
    va_list args;
    va_start(args, format);
    char buf[128];
    int len = vsnprintf(buf, sizeof(buf), format, args);
    if (len > 0) 
    {
        HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, 100);
    }
    va_end(args);
}
#endif
