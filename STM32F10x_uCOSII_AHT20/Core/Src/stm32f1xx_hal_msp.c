/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_hal_msp.c
  * @brief   MCU Support package：时钟、GPIO、USART1 底层初始化
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/**
  * @brief 初始化全局 MSP。
  */
void HAL_MspInit(void)
{
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /* 关闭 JTAG，保留 SWD，释放 PA15/PB3/PB4 */
  __HAL_AFIO_REMAP_SWJ_NOJTAG();
}

/**
  * @brief I2C1 底层初始化：PB6=SCL，PB7=SDA，复用开漏
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (hi2c->Instance == I2C1)
  {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == I2C1)
  {
    __HAL_RCC_I2C1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6 | GPIO_PIN_7);
  }
}

/**
  * @brief USART1 底层初始化
  * @note  PA9  -> USART1_TX
  *        PA10 -> USART1_RX
  *        本实验只用查询方式发送，不开启串口中断；优先级仍设为 6，
  *        避免以后添加串口中断时落入 uC/OS-II 的非内核感知优先级区间。
  */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (huart->Instance == USART1)
  {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA9：复用推挽输出 */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA10：浮空输入 */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(USART1_IRQn, 6u, 0u);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  }
}

/**
  * @brief USART1 底层反初始化
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if (huart->Instance == USART1)
  {
    __HAL_RCC_USART1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  }
}
