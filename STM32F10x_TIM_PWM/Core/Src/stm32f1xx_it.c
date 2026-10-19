/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   中断服务程序
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_it.h"

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim3;

/* -------------------- Cortex-M3 内核异常 -------------------- */
void NMI_Handler(void)        { while (1) { } }
void HardFault_Handler(void)  { while (1) { } }
void MemManage_Handler(void)  { while (1) { } }
void BusFault_Handler(void)   { while (1) { } }
void UsageFault_Handler(void) { while (1) { } }
void SVC_Handler(void)        { }
void DebugMon_Handler(void)   { }
void PendSV_Handler(void)     { }

/**
  * @brief 系统滴答 1ms
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

/**
  * @brief TIM3 更新中断
  * @note  HAL_TIM_IRQHandler 会清标志并回调 HAL_TIM_PeriodElapsedCallback()
  */
void TIM3_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim3);
}
