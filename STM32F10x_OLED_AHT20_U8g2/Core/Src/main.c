/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main.c
  * @brief   STM32F103C8T6 + SSD1306 OLED + AHT20 综合实验
  ******************************************************************************
  *  同一套 HAL 工程包含两个应用：
  *    TASK1_BUILD : 自定义 SSD1306 底层驱动 + 16x16 汉字 + AHT20 + 滚动
  *    TASK3_BUILD : 移植裁剪版 U8g2 + demo + 学号姓名 + 四向滚动 + 动画头像
  *
  *  硬件连接：
  *    OLED  VCC=3.3V GND=GND SCL=PB6 SDA=PB7
  *    AHT20 VCC=3.3V GND=GND SCL=PB6 SDA=PB7
  *    USB-TTL TX=PA10 RX=PA9 GND=GND（可选，115200）
  *    LED=PC13（低电平亮）  KEY=PA0（按下接地）
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "app_config.h"
#include "board.h"

#if defined(TASK3_BUILD)
#include "task3_app.h"
#else
#include "task1_app.h"
#endif

I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  Board_Init();

  Board_Print("\r\n========================================\r\n");
#if defined(TASK3_BUILD)
  Board_Print(" STM32F103C8T6 + SSD1306 + AHT20\r\n");
  Board_Print(" Task 3: U8g2 port / demo / scroll / avatar\r\n");
#else
  Board_Print(" STM32F103C8T6 + SSD1306 + AHT20\r\n");
  Board_Print(" Task 1: custom OLED driver / Chinese / scroll\r\n");
#endif
  Board_Print(" Student: 632407030612  Xu Zhengxin\r\n");
  Board_Print("========================================\r\n");

#if defined(TASK3_BUILD)
  Task3_Run();
#else
  Task1_Run();
#endif

  while (1)
  {
    Board_Service();
    HAL_Delay(100);
  }
}

/**
  * @brief 系统时钟：HSE 8MHz -> PLL x9 -> 72MHz
  */
static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO：PC13 LED、PA0 按键
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* PC13 推挽输出，默认高电平（LED 灭） */
  HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_SET);
  GPIO_InitStruct.Pin   = LED_PIN;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);

  /* PA0 上拉输入，按下为低 */
  GPIO_InitStruct.Pin  = KEY_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEY_GPIO_PORT, &GPIO_InitStruct);
}

/**
  * @brief I2C1：400kHz，7 位地址
  */
static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1：115200-8-N-1
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
    LED_TOGGLE();
    for (volatile uint32_t i = 0; i < 800000U; i++) { }
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
  while (1) { }
}
#endif
