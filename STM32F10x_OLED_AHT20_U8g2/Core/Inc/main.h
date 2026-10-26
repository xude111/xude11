/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main.h
  * @brief   公共头文件：时钟、I2C、USART、LED、按键定义
  ******************************************************************************
  * 硬件平台：STM32F103C8T6 最小系统板（Blue Pill）
  *   OLED 0.96" SSD1306  I2C1: PB6=SCL, PB7=SDA, 地址 0x3C (8 位写地址 0x78)
  *   AHT20 温湿度传感器 I2C1 同一总线，地址 0x38 (8 位写地址 0x70)
  *   USART1: PA9=TX, PA10=RX, 115200-8-N-1（调试输出，可选）
  *   LED: PC13，低电平点亮
  *   KEY: PA0，按下接地，内部上拉
  ******************************************************************************
  */
/* USER CODE END Header */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

/* 引脚定义 ----------------------------------------------------------------*/
#define LED_PIN             GPIO_PIN_13
#define LED_GPIO_PORT       GPIOC
#define KEY_PIN             GPIO_PIN_0
#define KEY_GPIO_PORT       GPIOA

#define LED_ON()            HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_RESET)
#define LED_OFF()           HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_SET)
#define LED_TOGGLE()        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN)
#define KEY_IS_DOWN()       (HAL_GPIO_ReadPin(KEY_GPIO_PORT, KEY_PIN) == GPIO_PIN_RESET)

/* 全局外设句柄 ------------------------------------------------------------*/
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;

/* 函数声明 ----------------------------------------------------------------*/
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
