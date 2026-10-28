/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : 公共头文件（引脚/函数声明）
  ******************************************************************************
  * @attention
  *
  * 实验任务：用 USART1 与上位机（Win10 串口助手）通信
  *   任务1）STM32 连续发送 "hello windows！"，发送之间不加延时
  *   任务2）上位机发 '#' → 暂停发送；发 '*' → 继续发送
  *
  * 串口参数：115200, 8 位数据位, 1 位停止位, 无校验（115200-8-N-1）
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
extern I2C_HandleTypeDef  hi2c1;
extern UART_HandleTypeDef huart1;

/* LED 定义：PA0 红灯、PB0 绿灯，高电平点亮 */
#define LED1_GPIO_PORT      GPIOA
#define LED1_PIN            GPIO_PIN_0
#define LED2_GPIO_PORT      GPIOB
#define LED2_PIN            GPIO_PIN_0
#define LED1_ON()           HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_SET)
#define LED1_OFF()          HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_RESET)
#define LED2_ON()           HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_SET)
#define LED2_OFF()          HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_RESET)

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
