#include "eventhub.h"
#include "stm32f1xx_hal.h"

// 定义事件类型
#define EVENT_POWER_ON 1
#define EVENT_POWER_OFF 2

// 全局事件中枢
static eventhub_t g_hub;

// 订阅者1：日志回调
void log_callback(const eventhub_event_t* event, void* user_data) 
{
    if (event->type == EVENT_POWER_ON) 
    {
        printf("VCC ON (time: %dms)\n", event->timestamp);
    }
}

// 订阅者2：LED回调
void led_callback(const eventhub_event_t* event, void* user_data) 
{
    if (event->type == EVENT_POWER_ON) 
    {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    }
}

int main(void) 
{
    // 初始化硬件
    HAL_Init();
    MX_USART1_UART_Init(); // 初始化UART（日志用）
    MX_GPIO_Init();        // 初始化GPIO（LED和VCC检测）

    // 初始化事件中枢（裸机模式）
    eventhub_init(&g_hub);

    // 订阅事件
    eventhub_subscribe(&g_hub, EVENT_POWER_ON, log_callback, NULL);
    eventhub_subscribe(&g_hub, EVENT_POWER_ON, led_callback, NULL);

    // 主循环
    while (1) 
    {
        // 检测VCC状态（假设PA0为VCC检测引脚）
        if (HAL_GPIO_ReadPin(GPIO_Port, Pin) == GPIO_PIN_SET) 
        {
            // 发布VCC打开事件
            eventhub_event_t event = 
            {
                .type = EVENT_POWER_ON,
                .data = NULL,
                .data_len = 0
            };
            eventhub_publish(&g_hub, &event, 0);

            // 等待VCC关闭（避免重复发布）
            while (HAL_GPIO_ReadPin(GPIO_Port, Pin) == GPIO_PIN_SET);
        }

        HAL_Delay(10); // 短延时
    }
}
