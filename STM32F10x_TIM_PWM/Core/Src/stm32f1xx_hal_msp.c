/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_hal_msp.c
  * @brief   外设底层初始化（时钟 + 引脚 + 中断）
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

void HAL_MspInit(void)
{
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();
}

/**
  * @brief TIM 底层初始化：TIM3 时钟、PA6(TIM3_CH1) 复用推挽、NVIC
  */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{
  if (htim_base->Instance == TIM3)
  {
    __HAL_RCC_TIM3_CLK_ENABLE();

    /* TIM3 更新中断（模式1 用） */
    HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
  }
}

/**
  * @brief PWM 通道底层初始化：PA6 复用推挽输出
  */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim_pwm)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (htim_pwm->Instance == TIM3)
  {
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA6 = TIM3_CH1，复用推挽输出 */
    GPIO_InitStruct.Pin   = GPIO_PIN_6;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base)
{
  if (htim_base->Instance == TIM3)
  {
    __HAL_RCC_TIM3_CLK_DISABLE();
    HAL_NVIC_DisableIRQ(TIM3_IRQn);
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
