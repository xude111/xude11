/*
*********************************************************************************************************
*                                     uC/OS-II 应用配置
* 工程：STM32F103C8T6 + uC/OS-II V2.93.01
*********************************************************************************************************
*/
#ifndef APP_CFG_H
#define APP_CFG_H

#include "os_cfg.h"

/* Cortex-M3 有 4 位可编程优先级，0 最高、15 最低。
   BASEPRI 边界设为 4：优先级 0~3 为不受内核临界区屏蔽的“非内核感知”中断，
   4~15 为内核感知中断，可以被 OS_ENTER_CRITICAL() 屏蔽。 */
#define CPU_CFG_KA_IPL_BOUNDARY       4u
#define CPU_CFG_NVIC_PRIO_BITS        4u

/* 任务优先级：数值越小优先级越高 */
#define APP_TASK_LED1_PRIO            5u
#define APP_TASK_LED2_PRIO            6u
#define APP_TASK_UART_PRIO            7u

/* 任务堆栈大小（单位：OS_STK，STM32 上 1 个 OS_STK = 4 字节） */
#define APP_TASK_LED1_STK_SIZE        256u
#define APP_TASK_LED2_STK_SIZE        256u
#define APP_TASK_UART_STK_SIZE        512u

#endif
