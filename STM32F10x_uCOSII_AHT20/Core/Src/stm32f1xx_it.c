/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   中断服务程序：uC/OS-II 的 SysTick 与 PendSV 接入
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_it.h"
#include "includes.h"

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart1;

/* -------------------- Cortex-M3 内核异常 -------------------- */
void NMI_Handler(void)
{
  while (1)
  {
  }
}

void HardFault_Handler(void)
{
  while (1)
  {
  }
}

void MemManage_Handler(void)
{
  while (1)
  {
  }
}

void BusFault_Handler(void)
{
  while (1)
  {
  }
}

void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief SVC 异常。uC/OS-II 的 Cortex-M3 移植使用 PendSV 做上下文切换，
  *        SVC 保持为空即可。
  */
void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

/**
  * @brief PendSV 异常：uC/OS-II 真正的上下文切换入口。
  */
void PendSV_Handler(void)
{
  OS_CPU_PendSVHandler();
}

/**
  * @brief SysTick 中断：1 ms 一次。
  *        先给 HAL 累加毫秒计数（HAL_UART_Transmit 的超时判断要用），
  *        再调用 uC/OS-II 的节拍处理函数。
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
  OS_CPU_SysTickHandler();
}

/* -------------------- 外设中断 -------------------- */
/**
  * @brief USART1 全局中断。本实验只用查询发送，因此该中断实际上不会触发；
  *        保留它是为了工程完整，便于以后扩展串口接收。
  */
void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart1);
}
