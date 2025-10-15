#include "eventhub.h"
#include "FreeRTOS.h"
#include "task.h"

// 事件类型
#define EVENT_VCC_POWER_ON 1

// 全局事件中枢
static eventhub_t g_hub;

// 订阅者回调
void network_callback(const eventhub_event_t* event, void* user_data) 
{
    if (event->type == EVENT_VCC_POWER_ON) 
    {
        printf("Network: VCC ON, starting connection...\n");
    }
}

// 事件处理任务（RTOS环境必须）
void event_process_task(void* param) 
{
    while (1) 
    {
        eventhub_process(&g_hub, portMAX_DELAY); // 阻塞等待事件
    }
}

// VCC检测任务（发布事件）
void VCC_detect_task(void* param) 
{
    while (1) 
    {
        // 模拟VCC检测（实际读取GPIO）
        bool VCC_on = true;
        if (VCC_on) 
        {
            eventhub_event_t event = {.type = EVENT_VCC_POWER_ON};
            eventhub_publish(&g_hub, &event, 0); // 非阻塞发布
            vTaskDelay(pdMS_TO_TICKS(1000)); // 避免频繁发布
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main(void) 
{
    // 初始化硬件（省略）

    // 初始化事件中枢（RTOS模式，需在eventhub_config.h中设置）
    eventhub_init(&g_hub);

    // 订阅事件
    eventhub_subscribe(&g_hub, EVENT_VCC_POWER_ON, network_callback, NULL);

    // 创建任务
    xTaskCreate(event_process_task, "event_process", 512, NULL, 3, NULL);
    xTaskCreate(VCC_detect_task, "VCC_detect", 256, NULL, 2, NULL);

    // 启动调度器
    vTaskStartScheduler();

    while (1); // 不会执行到这里
}
