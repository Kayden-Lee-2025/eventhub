# eventhub - 轻量级跨环境事件管理库

## 1. 项目介绍

`eventhub` 是一款专为嵌入式系统设计的轻量级事件管理库，采用**发布 - 订阅模式**实现模块间低耦合通信。核心特点是**跨环境兼容**，支持裸机（无操作系统）和 RTOS（如 FreeRTOS/RT-Thread/uC/OS）环境，用户可通过配置文件和适配层灵活适配不同硬件平台与操作系统。

### 1.1 核心优势

- **轻量级**：无动态内存依赖，资源占用可控（RAM < 2KB，ROM < 5KB），适合 MCU 资源受限场景。
- **跨环境**：一键切换「裸机 / RTOS」模式，RTOS 环境支持异步事件队列，裸机环境支持同步回调。
- **灵活适配**：平台接口（如互斥锁、队列、时间戳）由用户定义，支持任意 RTOS 或裸机硬件。
- **线程安全**：RTOS 环境通过互斥锁保护订阅者列表，支持多任务并发发布 / 订阅。
- **可扩展**：支持自定义事件类型、附加数据，最大订阅者 / 队列大小可配置。

## 2. 快速开始

### 2.1 环境准备

- **硬件**：任意 32 位 MCU（如 STM32、ESP32、GD32 等）。
- **工具链**：GCC-ARM、Keil MDK、IAR 等嵌入式编译器。
- **RTOS（可选）**：FreeRTOS、RT-Thread、uC/OS-III 等（裸机环境无需）。

### 2.2 库文件结构

```plaintext
eventhub/
├── include/                  # 头文件目录
│   ├── eventhub.h            # 核心 API 声明（用户调用）
│   ├── eventhub_config.h     # 配置文件（用户修改）
│   └── eventhub_port.h       # 平台适配接口（用户实现）
├── src/                      # 源文件目录
│   ├── eventhub_core.c       # 核心逻辑（发布/订阅/事件分发）
│   └── port/                 # 适配层示例（用户参考）
│       ├── baremetal/        # 裸机适配示例
│       │   └── eventhub_port.c
│       └── freertos/         # FreeRTOS 适配示例
│           └── eventhub_port.c
├── examples/                 # 使用示例
│   ├── baremetal_demo.c      # 裸机环境示例
│   └── freertos_demo.c       # FreeRTOS 环境示例
└── LICENSE                   # MIT 许可证
```



### 2.3 集成步骤

1. #### 步骤 1：添加文件到项目

   1. 将 `include/` 下的 3 个头文件添加到项目头文件路径。
   2. 将 `src/eventhub_core.c` 添加到项目源文件。
   3. 根据运行环境，参考 `src/port/` 下的示例，实现 `eventhub_port.c`（平台适配接口）。

   #### 步骤 2：配置 `eventhub_config.h`

   根据项目需求修改配置（关键配置项说明如下）：

   ```c
   // 1. 选择运行环境：0=裸机，1=RTOS
   #define EVENTHUB_USING_RTOS 0
   
   // 2. 资源限制（根据 MCU 内存调整）
   #define EVENTHUB_MAX_EVENT_TYPES 10  // 最大事件类型数
   #define EVENTHUB_MAX_SUBSCRIBERS 8   // 最大订阅者数
   #define EVENTHUB_QUEUE_SIZE 16       // RTOS 事件队列大小（裸机忽略）
   
   // 3. 调试开关（1=启用日志，0=禁用）
   #define EVENTHUB_ENABLE_LOG 1
   ```

   #### 步骤 3：实现平台适配接口（`eventhub_port.c`）

   需实现 `eventhub_port.h` 中声明的接口，不同环境的核心接口差异如下：



| 环境 | 需实现的核心接口                                             |
| ---- | ------------------------------------------------------------ |
| 裸机 | 互斥锁（空实现）、时间戳（如 SysTick）、日志（如 UART）      |
| RTOS | 互斥锁（如 FreeRTOS Semaphore）、队列（如 FreeRTOS Queue）、时间戳、日志 |



   **示例（裸机环境时间戳实现）**：

   ```c
   // 使用 SysTick 定时器生成 1ms 时间戳
   static volatile uint32_t sys_tick_ms = 0;
   void SysTick_Handler(void) 
   {
       sys_tick_ms++;
   }
   
   eventhub_timestamp_t eventhub_port_get_timestamp(void) 
   {
       return sys_tick_ms;
   }
   ```

   **示例（FreeRTOS 队列实现）**：

   ```c
   bool eventhub_port_queue_send(eventhub_queue_t* queue, const void* data, uint32_t timeout) 
   {
       return xQueueSend(queue, data, timeout) == pdPASS;
   }
   ```

   ## 3. 核心 API 说明

   所有 API 定义在 `eventhub.h` 中，接口风格统一，屏蔽底层环境差异。

   ### 3.1 事件中枢初始化与销毁

| 函数原型                                 | 功能描述                                                     |
| ---------------------------------------- | ------------------------------------------------------------ |
| `bool eventhub_init(eventhub_t* hub)`    | 初始化事件中枢（需传入全局 `eventhub_t` 实例），成功返回 `true`。 |
| `void eventhub_destroy(eventhub_t* hub)` | 销毁事件中枢（释放互斥锁、队列等资源），裸机环境可省略。     |

   ### 3.2 事件订阅与取消订阅

| 函数原型                                                     | 功能描述                                                    |
| ------------------------------------------------------------ | ----------------------------------------------------------- |
| `bool eventhub_subscribe(eventhub_t* hub, eventhub_event_type_t event_type, eventhub_subscriber_cb cb, void* user_data)` | 订阅指定类型事件，传入回调函数和用户数据，成功返回 `true`。 |
| `bool eventhub_unsubscribe(eventhub_t* hub, eventhub_event_type_t event_type, eventhub_subscriber_cb cb)` | 取消订阅指定事件类型的回调函数，成功返回 `true`。           |

   ### 3.3 事件发布与处理

| 函数原型                                                     | 功能描述                                                     |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| `bool eventhub_publish(eventhub_t* hub, const eventhub_event_t* event, uint32_t timeout)` | 发布事件：- 裸机环境：同步调用所有订阅者回调；- RTOS 环境：将事件放入队列（`timeout` 为等待时间）。 |
| `void eventhub_process(eventhub_t* hub, uint32_t timeout)`   | 事件处理：- 裸机环境：空实现（发布时已同步处理）；- RTOS 环境：从队列取事件并分发（需在独立任务中调用）。 |

   ### 3.4 事件数据结构

   ```c
   // 事件类型（用户需扩展具体类型，如 EVENT_VCC_POWER_ON）
   typedef uint8_t eventhub_event_type_t;
   
   // 事件数据结构体
   typedef struct 
   {
       eventhub_event_type_t type;          // 事件类型（如 1=VCC电源打开）
       eventhub_timestamp_t timestamp;      // 事件时间戳（ms）
       void* data;                          // 附加数据（可选，如电压值、GPS坐标）
       uint32_t data_len;                   // 附加数据长度（字节）
   } eventhub_event_t;
   ```

   ## 4. 环境适配指南

   ### 4.1 裸机环境适配

   裸机环境无多任务调度，核心是**同步回调**和**静态内存**：



   1. **互斥锁**：无需实际锁，接口返回 `true` 即可（示例见 `src/port/baremetal/eventhub_port.c`）。
   2. **时间戳**：使用 SysTick 或定时器生成 1ms 精度时间（需在中断服务函数中更新）。
   3. **事件处理**：发布事件时直接调用订阅者回调，回调函数需**简短无阻塞**（避免影响主循环）。



   **裸机适配关键点**：



   - 不支持事件队列，所有事件同步处理。
   - 订阅者回调函数不能包含 `delay` 或耗时操作。

   ### 4.2 RTOS 环境适配

   RTOS 环境支持**异步事件队列**，核心是**任务间通信**：



   1. **互斥锁**：使用 RTOS 提供的互斥锁（如 FreeRTOS `xSemaphoreCreateMutex`）。
   2. **队列**：使用 RTOS 提供的队列（如 FreeRTOS `xQueueCreate`），存储事件数据。
   3. **事件处理任务**：需创建独立任务，循环调用 `eventhub_process` 从队列取事件并分发。



   **RTOS 适配关键点**：



   - 事件发布时仅将事件放入队列，不阻塞发布者任务。
   - 事件处理任务优先级建议设为中等（避免抢占核心业务任务）。

   ## 5. 完整使用示例

   ### 5.1 裸机环境示例（STM32 为例）

   #### 步骤 1：定义事件类型与回调

   ```c
   #include "eventhub.h"
   #include "stm32f1xx_hal.h"
   
   // 1. 定义事件类型
   #define EVENT_VCC_POWER_ON  1  // VCC电源打开事件
   #define EVENT_VCC_POWER_OFF 2  // VCC电源关闭事件
   
   // 2. 全局事件中枢实例
   static eventhub_t g_hub;
   
   // 3. 订阅者1：日志回调（通过 UART 打印）
   void log_callback(const eventhub_event_t* event, void* user_data) 
   {
       if (event->type == EVENT_VCC_POWER_ON) 
       {
           printf("VCC ON - Timestamp: %dms\n", event->timestamp);
       }
   }
   
   // 4. 订阅者2：LED 控制回调
   void led_callback(const eventhub_event_t* event, void* user_data) 
   {
       if (event->type == EVENT_VCC_POWER_ON) 
       {
           HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);  // 点亮 LED
       } 
       else if (event->type == EVENT_VCC_POWER_OFF) 
       {
           HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET); // 熄灭 LED
       }
   }
   ```

   #### 步骤 2：主循环发布事件

   ```c
   int main(void) 
   {
       // 初始化硬件（UART、GPIO、SysTick）
       HAL_Init();
       MX_USART1_UART_Init();  // 初始化 UART（日志输出）
       MX_GPIO_Init();          // 初始化 GPIO（LED 和 VCC 检测）
   
       // 初始化事件中枢
       if (!eventhub_init(&g_hub)) 
       {
           printf("Eventhub init failed!\n");
           while (1);
       }
   
       // 订阅事件
       eventhub_subscribe(&g_hub, EVENT_VCC_POWER_ON, log_callback, NULL);
       eventhub_subscribe(&g_hub, EVENT_VCC_POWER_ON, led_callback, NULL);
       eventhub_subscribe(&g_hub, EVENT_VCC_POWER_OFF, led_callback, NULL);
   
       // 主循环：检测 VCC 状态并发布事件
       while (1) 
       {
           // 检测 VCC 引脚（假设 PB0 为 VCC 检测引脚）
           if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET) 
           {
               // 发布 VCC 打开事件
               eventhub_event_t event = 
               {
                   .type = EVENT_VCC_POWER_ON,
                   .data = NULL,
                   .data_len = 0
               };
               eventhub_publish(&g_hub, &event, 0);
   
               // 等待 VCC 关闭（避免重复发布）
               while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET);
           } 
           else 
           {
               // 发布 VCC 关闭事件
               eventhub_event_t event = {
                   .type = EVENT_VCC_POWER_OFF,
                   .data = NULL,
                   .data_len = 0
               };
               eventhub_publish(&g_hub, &event, 0);
           }
   
           HAL_Delay(10);  // 短延时，降低 CPU 占用
       }
   }
   ```

   ### 5.2 FreeRTOS 环境示例

   #### 步骤 1：创建事件处理任务

   ```c
   #include "eventhub.h"
   #include "FreeRTOS.h"
   #include "task.h"
   
   // 1. 定义事件类型
   #define EVENT_VCC_POWER_ON 1
   
   // 2. 全局事件中枢实例
   static eventhub_t g_hub;
   
   // 3. 订阅者回调：网络模块启动
   void network_callback(const eventhub_event_t* event, void* user_data) 
   {
       if (event->type == EVENT_VCC_POWER_ON) 
       {
           printf("Network: VCC ON, starting LTE connection...\n");
           // 实际业务：初始化 LTE 模块
       }
   }
   
   // 4. 事件处理任务（RTOS 核心任务）
   void event_process_task(void* param) 
   {
       while (1) 
       {
           // 阻塞等待事件（无事件时释放 CPU）
           eventhub_process(&g_hub, portMAX_DELAY);
       }
   }
   
   // 5. VCC 检测任务（发布事件）
   void vcc_detect_task(void* param) 
   {
       while (1) {
           // 模拟 VCC 检测（实际读取 GPIO）
           bool vcc_on = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET;
           if (Vcc_on) 
           {
               eventhub_event_t event = 
               {
                   .type = EVENT_VCC_POWER_ON,
                   .data = NULL,
                   .data_len = 0
               };
               // 非阻塞发布事件（队列满时直接返回）
               eventhub_publish(&g_hub, &event, 0);
               vTaskDelay(pdMS_TO_TICKS(1000));  // 避免频繁发布
           }
           vTaskDelay(pdMS_TO_TICKS(100));  // 100ms 检测一次
       }
   }
   ```

   #### 步骤 2：主函数初始化与任务创建

   ```c
   int main(void) 
   {
       // 初始化硬件（UART、GPIO、LTE 模块）
       HAL_Init();
       MX_USART1_UART_Init();
       MX_GPIO_Init();
   
       // 初始化事件中枢（RTOS 模式，需在 eventhub_config.h 中设置 EVENTHUB_USING_RTOS=1）
       if (!eventhub_init(&g_hub)) 
       {
           printf("Eventhub init failed!\n");
           while (1);
       }
   
       // 订阅事件
       eventhub_subscribe(&g_hub, EVENT_VCC_POWER_ON, network_callback, NULL);
   
       // 创建 FreeRTOS 任务
       xTaskCreate(
           event_process_task,  // 事件处理任务
           "EventProcess",      // 任务名
           512,                 // 栈大小（words）
           NULL,                // 任务参数
           3,                   // 优先级（中等）
           NULL
       );
   
       xTaskCreate(
           vcc_detect_task,     // VCC 检测任务
           "VccDetect",         // 任务名
           256,                 // 栈大小（words）
           NULL,                // 任务参数
           2,                   // 优先级（低于事件处理任务）
           NULL
       );
   
       // 启动 FreeRTOS 调度器
       vTaskStartScheduler();
   
       // 调度器启动失败（仅当内存不足时执行）
       while (1);
   }
   ```

### 5.3 简化函数调用API

如果整个项目都只有一个eventhub实例，那么每次发布订阅都传入eventhub实例就会增加函数调用的麻烦，必须先声明eventhub实例，才能调用发布订阅的API显得多余。

基于此，如果是单例模式，用户可以在应用层再增加一次接口封装，如增加一个app_evevthub.h文件，实现示例如下：

```c
// app_evevthub.h
#include "eventhub.h"


extern eventhub_t g_eventhub;	// 假设这是在应用中定义的实例

#define APP_EVENTHUB_SUBSCRIBE(event_type, cb, user_data) \
    eventhub_subscribe(&g_eventhub, event_type, cb, user_data)
    
#define APP_EVENTHUB_UNSUBSCRIBE(event_type, cb) \
    eventhub_unsubscribe(&g_default_eventhub, event_type, cb)

#define APP_EVENTHUB_PUBLISH(event, timeout) \
    eventhub_publish(&g_default_eventhub, event, timeout)

#define APP_EVENTHUB_PROCESS(timeout) \
    eventhub_process(&g_default_eventhub, timeout)
```

这样用户调用宏定义的简化API时就可以少传递一个参数，应用只需要引用这个头文件即可。

   ## 6. 常见问题与调试

   ### 6.1 问题 1：事件发布后订阅者未收到回调

   - 排查步骤

     ：

     1. 检查 `eventhub_config.h` 中 `EVENTHUB_USING_RTOS` 是否与实际环境一致。
     2. RTOS 环境：确认 `event_process_task` 已创建且优先级正确（需高于发布任务）。
     3. 裸机环境：确认回调函数未包含阻塞操作（如 `HAL_Delay`）。
     4. 检查订阅时的 `event_type` 与发布时是否一致。

   ### 6.2 问题 2：RTOS 环境下队列满导致发布失败

   - 解决方案

     ：

     1. 增大 `eventhub_config.h` 中的 `EVENTHUB_QUEUE_SIZE`（如从 16 改为 32）。
     2. 发布事件时设置非零 `timeout`（如 `pdMS_TO_TICKS(100)`），允许短暂等待队列空闲。
     3. 降低事件发布频率（如在发布后添加 `vTaskDelay`）。

   ### 6.3 问题 3：裸机环境下主循环卡顿

   - **原因**：订阅者回调函数耗时过长（如复杂计算、UART 阻塞发送）。

   - 解决方案

     ：

     1. 回调函数仅做 “事件接收”，耗时操作放到主循环单独处理（如通过标志位）。
     2. UART 日志使用 DMA 发送，避免阻塞回调函数。

   ## 7. 许可证

   本项目采用 **MIT 许可证**，允许个人和商业项目自由使用、修改和分发，无需支付费用，只需保留原许可证声明。

   ```
   MIT License
   
   Copyright (c) 2025 Kayden
   
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
   ```

   ## 8. 贡献指南

   欢迎通过以下方式贡献代码或反馈问题：



   1. **提交 Issue**：在 GitHub 仓库提交 bug 报告或功能建议。
   2. **提交 PR**：Fork 仓库后修改代码，提交 Pull Request（需遵循代码风格）。
   3. **适配扩展**：新增 RTOS（如 RT-Thread）或硬件平台的适配层，提交到 `src/port/` 目录。



   代码风格要求：

   - 变量 / 函数名使用小写蛇形命名（如 `eventhub_init`）。
   - 注释需清晰说明函数功能、参数含义和返回值。
   - 避免使用动态内存（`malloc/free`），优先使用静态数组。
