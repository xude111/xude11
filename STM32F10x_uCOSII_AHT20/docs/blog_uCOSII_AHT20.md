# STM32F103 移植 uC/OS-II：从三个 LED 任务到 AHT20 温湿度采集

> 课程：嵌入式系统  
> 平台：STM32F103C8T6 + Keil MDK 5.06 + uC/OS-II V2.93.01  
> 作者：徐正昕（632407030612，通信 2403 班）  
> 代码：<https://github.com/xude111/xude11/tree/main/STM32F10x_uCOSII_AHT20>

---

## 1. 实验目标

这次实验分两部分：

1. **RTOS 移植**：把 uC/OS-II 移植到 STM32F103C8T6，建立 3 个任务：
   - Task_LED1：PA0 红灯，1 s 周期（亮 0.5 s、灭 0.5 s）；
   - Task_LED2：PB0 绿灯，3 s 周期（亮 1.5 s、灭 1.5 s）；
   - Task_AHT20：每 2 s 通过 I2C1 读取 AHT20 温湿度，并从 USART1 发送。
2. **I2C 与传感器**：解释软件 I2C 与硬件 I2C 的区别，阅读 AHT20 数据手册，完成初始化、触发测量、7 字节读取、温湿度换算和串口输出。

最终工程：

```text
Code=13928  RO-data=568  RW-data=64  ZI-data=7224
0 Error(s), 0 Warning(s)
HEX: 41001 字节
```

---

## 2. 为什么选择 uC/OS-II

uC/OS-II 是一个抢占式、基于优先级的实时内核，代码规模小、结构清晰，适合在 STM32F103 这类资源有限的 Cortex-M3 上学习 RTOS 的基本机制：

- 任务控制块 TCB 和就绪表；
- 优先级调度；
- 系统节拍和延时；
- 临界区与上下文切换；
- 可裁剪的内核服务。

工程中只保留任务管理、时间管理和调度器，关闭事件标志、邮箱、内存管理、互斥量、队列、信号量和软件定时器，减少 Flash 和 RAM 占用。

---

## 3. 移植前的目录规划

以已经调通的 HAL USART 工程为基础，复制为 `STM32F10x_uCOSII_RTOS`，新增：

```text
Middlewares/uCOSII/
├─ Source/      uC/OS-II 内核源码
├─ Ports/       Cortex-M3/ARMCC 移植层
└─ Cfg/         os_cfg.h、app_cfg.h、includes.h

App/
├─ app.c        三个应用任务
├─ aht20.c      AHT20 驱动
└─ aht20.h
```

移植层选用 `Ports/ARM-Cortex-M/ARMv7-M/ARM`，对应 Cortex-M3 + ARMCC（AC5）：

- `os_cpu.h`：数据类型、`OS_CRITICAL_METHOD`、`OS_STK_GROWTH`、`OS_TASK_SW`；
- `os_cpu_a.asm`：`OSStartHighRdy`、`OSCtxSw`、`OSIntCtxSw`、`OS_CPU_PendSVHandler`、`OS_CPU_SR_Save/Restore`；
- `os_cpu_c.c`：`OSInitHookBegin`、`OSTaskSwHook`、`OS_CPU_SysTickHandler`、`OS_CPU_SysTickInitFreq`；
- `os_dbg.c`：调试变量。

> 注意：`ucos_ii.c` 是“合集文件”，和逐个加入 `.c` 的方式二选一，不能同时加入，否则会重复定义。

---

## 4. 关键配置

### 4.1 os_cfg.h

```c
#define OS_MAX_TASKS             8u
#define OS_LOWEST_PRIO          15u
#define OS_TICKS_PER_SEC      1000u
#define OS_TASK_IDLE_STK_SIZE  128u
#define OS_TASK_STAT_EN          0u
#define OS_TASK_CREATE_EN        1u
#define OS_TASK_CREATE_EXT_EN    0u
#define OS_TASK_SW_HOOK_EN       1u
#define OS_TIME_DLY_HMSM_EN      1u
#define OS_TMR_EN                0u
```

几个容易踩坑的宏：

- `OS_TICKS_PER_SEC=1000`：1 ms 一个节拍，`OSTimeDlyHMSM` 的毫秒参数才准确；
- `OS_TASK_SW_HOOK_EN=1`：汇编 `OSStartHighRdy` 会调用 `OSTaskSwHook`，设 0 会链接失败；
- `OS_TIME_DLY_HMSM_EN=1`：否则 `OSTimeDlyHMSM` 未定义；
- `OS_TASK_STAT_EN=0`：关闭统计任务，少一个任务和一段代码。

### 4.2 app_cfg.h

```c
#define CPU_CFG_KA_IPL_BOUNDARY   4u
#define CPU_CFG_NVIC_PRIO_BITS    4u

#define APP_TASK_LED1_PRIO        5u
#define APP_TASK_LED2_PRIO        6u
#define APP_TASK_UART_PRIO        7u

#define APP_TASK_LED1_STK_SIZE  256u
#define APP_TASK_LED2_STK_SIZE  256u
#define APP_TASK_UART_STK_SIZE  512u
```

Cortex-M3 的 NVIC 有 4 位优先级。BASEPRI 边界设为 4：

- 优先级 0～3：非内核感知中断，不受 `OS_ENTER_CRITICAL()` 屏蔽；
- 优先级 4～15：内核感知中断，会被临界区屏蔽。

SysTick 使用 4，USART1 使用 6，都在内核感知范围内。

### 4.3 includes.h

```c
#ifndef INCLUDES_H
#define INCLUDES_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "stm32f1xx_hal.h"
#include "app_cfg.h"
#include "os_cpu.h"
#include "ucos_ii.h"
#endif
```

`app_cfg.h` 必须在 `os_cpu.h` 之前包含，因为 `os_cpu.h` 会在编译期检查 `CPU_CFG_KA_IPL_BOUNDARY` 和 `CPU_CFG_NVIC_PRIO_BITS`。

---

## 5. 启动流程与 HAL 时基

`HAL_Init()` 会先把 SysTick 配成 1 ms 并打开中断，但此时 uC/OS-II 还没初始化。如果 SysTick 直接进入 `OS_CPU_SysTickHandler()`，可能在 TCB 尚未建立时调用 `OSTimeTick()`，导致 HardFault。

正确的初始化顺序：

```c
HAL_Init();
__disable_irq();            /* OSInit 完成前关闭全局中断 */

SystemClock_Config();       /* 72 MHz */
MX_GPIO_Init();
MX_I2C1_Init();
MX_USART1_UART_Init();

OSInit();
App_Init();
OS_CPU_SysTickInitFreq(SystemCoreClock);
OSStart();                  /* 开中断，启动第一个任务 */
```

`SysTick_Handler` 同时服务 HAL 和 uC/OS-II：

```c
void SysTick_Handler(void)
{
    HAL_IncTick();              /* HAL 的 1 ms 计数 */
    OS_CPU_SysTickHandler();    /* uC/OS-II 的节拍 */
}
```

`HAL_IncTick()` 不能省：`HAL_UART_Transmit` 的超时判断依赖 `HAL_GetTick()`。

`PendSV_Handler` 才是任务切换入口：

```c
void PendSV_Handler(void)
{
    OS_CPU_PendSVHandler();
}
```

---

## 6. Cortex-M3 上下文切换

Cortex-M3 有两个堆栈指针：

- MSP：异常/中断使用；
- PSP：任务使用。

发生异常时，硬件自动把 R0～R3、R12、LR、PC、xPSR 压入当前堆栈；PendSV 处理程序再保存 R4～R11 和 LR，并切换 PSP，从而完成任务上下文切换。

PendSV 被设为最低优先级，保证它在所有其他中断处理完之后执行，避免在中断嵌套中切换任务。

临界区由 `OS_CPU_SR_Save/Restore` 操作 BASEPRI 实现：

```asm
OS_CPU_SR_Save
    CPSID   I
    PUSH    {R1}
    MRS     R1, BASEPRI
    MSR     BASEPRI, R0
    DSB
    ISB
    MOV     R0, R1
    POP     {R1}
    CPSIE   I
    BX      LR
```

---

## 7. 三个任务

```c
static void App_TaskLed1(void *p_arg)
{
    (void)p_arg;
    for (;;) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
        OSTimeDlyHMSM(0u, 0u, 0u, 500u);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        OSTimeDlyHMSM(0u, 0u, 0u, 500u);
    }
}
```

```c
static void App_TaskLed2(void *p_arg)
{
    (void)p_arg;
    for (;;) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
        OSTimeDlyHMSM(0u, 0u, 1u, 500u);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
        OSTimeDlyHMSM(0u, 0u, 1u, 500u);
    }
}
```

第三个任务先初始化 AHT20，然后每 2 s 读取并发送：

```c
static void App_TaskAht20(void *p_arg)
{
    float temperature = 0.0f;
    float humidity = 0.0f;
    int len;

    (void)p_arg;

    if (AHT20_Init() != 0u) {
        const char *err = "AHT20 init failed, check PB6/PB7 and 3.3V\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t *)err, strlen(err), 1000u);
    }

    for (;;) {
        if (AHT20_Read(&temperature, &humidity) == 0u) {
            len = snprintf(g_msg, sizeof(g_msg),
                "hello uc/OS! AHT20: T=%.2f C, H=%.2f %%RH\r\n",
                temperature, humidity);
        } else {
            len = snprintf(g_msg, sizeof(g_msg),
                "hello uc/OS! AHT20 read error\r\n");
        }
        if (len > 0) {
            HAL_UART_Transmit(&huart1, (uint8_t *)g_msg, len, 1000u);
        }
        OSTimeDlyHMSM(0u, 0u, 2u, 0u);
    }
}
```

串口输出示例：

```text
hello uc/OS! AHT20: T=25.34 C, H=60.12 %RH
hello uc/OS! AHT20: T=25.36 C, H=60.08 %RH
```

---

## 8. 软件 I2C 与硬件 I2C

| 对比项 | 软件 I2C | 硬件 I2C |
|---|---|---|
| 实现方式 | GPIO 翻转模拟 SCL/SDA | STM32 片内 I2C 外设 |
| 引脚 | 任意 GPIO | 固定复用引脚，常用 PB6/PB7 |
| 速率 | 通常 100 kHz 以下 | 100/400 kHz，硬件保证 |
| CPU 占用 | 高，逐位延时和翻转 | 低，可用中断/DMA |
| 可靠性 | 依赖延时，易被中断打断 | 状态机完整，错误标志清晰 |
| 适用场景 | 引脚受限、教学演示 | 正式产品、多从机、高速场景 |

本实验使用硬件 I2C1：

- PB6 = SCL，PB7 = SDA；
- 400 kHz，7 位地址；
- AHT20 7 位地址 `0x38`，HAL 使用 8 位写地址 `0x70`；
- 模块通常自带 4.7 kΩ 上拉；裸传感器必须外接上拉。

I2C1 初始化：

```c
hi2c1.Instance = I2C1;
hi2c1.Init.ClockSpeed = 400000;
hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
hi2c1.Init.OwnAddress1 = 0;
hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
hi2c1.Init.OwnAddress2 = 0;
hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
HAL_I2C_Init(&hi2c1);
```

---

## 9. AHT20 数据手册要点

AHT20 的测量流程：

1. 上电等待约 40 ms；
2. 读状态字节，若 bit3（CAL）为 0，发送初始化命令 `0xBE 0x08 0x00`，等待 10 ms；
3. 发送触发测量命令 `0xAC 0x33 0x00`，等待约 80 ms；
4. 读取 7 字节：状态 + 湿度 20 bit + 温度 20 bit + CRC；
5. 检查状态 bit7（BUSY），为 0 表示转换完成。

湿度：

```text
RH(%) = raw_h × 100 / 2^20
```

温度：

```text
T(℃) = raw_t × 200 / 2^20 − 50
```

`raw_h` 由 `buf[1]`～`buf[3]` 的高 20 位组成；`raw_t` 由 `buf[3]` 的低 4 位和 `buf[4]`、`buf[5]` 组成。

AHT20 驱动中的等待全部改成 `OSTimeDlyHMSM`：

```c
static void AHT20_DelayMs(uint32_t ms)
{
    OSTimeDlyHMSM(0u, 0u, 0u, (INT16U)ms);
}
```

这样 40 ms、10 ms、80 ms 的等待期间任务进入阻塞态，LED 任务仍能按时运行，不会因为传感器转换而掉帧。

---

## 10. 编译中遇到的问题

| 现象 | 原因 | 解决 |
|---|---|---|
| Keil 提示 Cannot read project file | uvprojx 保存成了带 BOM 的 UTF-8 | 用无 BOM 的 UTF-8 重新保存 |
| 找不到 `ucos_ii.h` | 未加入 Cfg/Source/Ports/App 头文件路径 | 补齐 Include Paths |
| `L6218E: Undefined symbol OSTaskSwHook` | `OS_TASK_SW_HOOK_EN=0` | 改为 1 |
| `OS_CPU_FP_Reg_Push/Pop` 未定义 | Cortex-M3 无 FPU，端口宏不一致 | 强制 `OS_CPU_ARM_FP_EN=0` |
| 程序跑飞 / HardFault | SysTick 在 OSInit 前进入内核 | OSInit 前 `__disable_irq()`，PendSV 接入 |
| 串口超时不返回 | `HAL_GetTick()` 没有递增 | SysTick 中调用 `HAL_IncTick()` |
| `HAL_I2C_Init` 链接失败 | 未打开 `HAL_I2C_MODULE_ENABLED` | 打开宏并加入 `stm32f1xx_hal_i2c.c` |
| AHT20 一直 `HAL_ERROR` | 地址写成 0x38、未接上拉或未供电 | 用 0x70；检查 3.3 V 和 4.7 kΩ 上拉 |
| 数据全 0 或 BUSY 一直为 1 | 触发后等待不足 80 ms | 保证 40/10/80 ms 延时，读状态 bit7 |

---

## 11. 资源占用

```text
Code=13928  RO-data=568  RW-data=64  ZI-data=7224
0 Error(s), 0 Warning(s)
HEX: 41001 字节
```

ZI-data 约 7.1 KB，包含三个任务堆栈、空闲任务堆栈、异常栈、内核全局变量和 AHT20 驱动静态变量。STM32F103C8 有 20 KB SRAM，余量充足。

---

## 12. 总结

这次实验把“RTOS 移植”和“I2C 传感器采集”串了起来：

- uC/OS-II 的移植核心是 SysTick、PendSV、BASEPRI 和 PSP/MSP；
- HAL 与 uC/OS-II 共用 SysTick 时，必须同时调用 `HAL_IncTick()` 和 `OS_CPU_SysTickHandler()`；
- AHT20 的 80 ms 转换等待应使用 `OSTimeDlyHMSM`，而不是 `HAL_Delay`；
- 硬件 I2C 比软件 I2C 更稳定、CPU 占用更低，但要注意引脚复用、上拉电阻和 8 位地址；
- 三个任务通过优先级和阻塞延时协同运行，LED 不会因为传感器读取或串口发送而停止闪烁。

完整工程和报告见 GitHub：

<https://github.com/xude111/xude11/tree/main/STM32F10x_uCOSII_AHT20>
