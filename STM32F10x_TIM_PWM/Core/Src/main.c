/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : TIM3 定时器 —— 定时中断闪灯 + PWM 呼吸灯
  ******************************************************************************
  * 实验内容：
  *   模式1  TIM_MODE = 1：TIM3 定时中断，每 1 s 进一次中断翻转 LED，
  *                         LED 以 2 s 的周期亮—灭
  *   模式2  TIM_MODE = 2：TIM3_CH1(PA6) 输出 PWM，占空比 0→100%→0 缓慢变化，
  *                         LED 呈呼吸灯效果，一个完整呼吸周期约 2 s
  *
  * 硬件连接：
  *   模式1：直接用板载 LED（PC13，低电平点亮）
  *   模式2：PA6(TIM3_CH1) → 220Ω → LED → GND
  *
  * 时钟：系统 72 MHz，TIM3 挂在 APB1(36 MHz)，
  *       但 APB1 分频≠1 时定时器时钟 = 2×PCLK1 = 72 MHz
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* ★ 1 = 定时中断 2s 闪灯   2 = PWM 呼吸灯 ★ */
#define TIM_MODE          2U
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */
#define LED_PORT   GPIOC
#define LED_PIN    GPIO_PIN_13          /* 板载 LED，低电平点亮 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */
#if (TIM_MODE == 1U)
  /* 模式1：启动定时器"更新中断"，溢出频率 1 Hz */
  if (HAL_TIM_Base_Start_IT(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
#else
  /* 模式2：启动 TIM3_CH1 的 PWM 输出 */
  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
#endif
  /* USER CODE END 2 */

  /* USER CODE BEGIN WHILE */
  while (1)
  {
#if (TIM_MODE == 2U)
    /* ============ 呼吸灯：占空比线性变化 ============
       ARR = 999，CCR 从 0 增加到 999 再减回 0，
       每 1 ms 改一次 → 上升 1 s + 下降 1 s = 2 s 一个呼吸周期 */
    uint16_t i;
    for (i = 0U; i <= 999U; i++)
    {
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, i);
      HAL_Delay(1U);
    }
    for (i = 999U; i > 0U; i--)
    {
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, i);
      HAL_Delay(1U);
    }
#endif
  }
  /* USER CODE END WHILE */
}

/**
  * @brief 系统时钟 72 MHz
  */
void SystemClock_Config(void)
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
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) { Error_Handler(); }
}

/**
  * @brief TIM3 初始化
  * @note  模式1：PSC=7199, ARR=9999 → 72MHz/7200=10kHz, 10kHz/10000=1Hz 中断
  *        模式2：PSC=71,   ARR=999  → 72MHz/72=1MHz,  1MHz/1000 = 1kHz PWM
  */
static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef  sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
#if (TIM_MODE == 2U)
  TIM_OC_InitTypeDef      sConfigOC = {0};
#endif

  htim3.Instance = TIM3;
#if (TIM_MODE == 1U)
  htim3.Init.Prescaler = 7200U - 1U;      /* 10 kHz 计数 */
  htim3.Init.Period    = 10000U - 1U;     /* 1 s 溢出一次 */
#else
  htim3.Init.Prescaler = 72U - 1U;        /* 1 MHz 计数 */
  htim3.Init.Period    = 1000U - 1U;      /* PWM 频率 1 kHz */
#endif
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK) { Error_Handler(); }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) { Error_Handler(); }

#if (TIM_MODE == 2U)
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) { Error_Handler(); }

  /* PWM 模式1：CNT < CCR 时输出高电平 */
  sConfigOC.OCMode     = TIM_OCMODE_PWM1;
  sConfigOC.Pulse      = 0;               /* 初始占空比 0 */
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) { Error_Handler(); }
#endif

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) { Error_Handler(); }
}

/**
  * @brief GPIO 初始化
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* PC13 板载 LED：推挽输出，初始熄灭（高电平） */
  HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
  GPIO_InitStruct.Pin   = LED_PIN;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

  /* PA6 由 HAL_TIM_PWM_MspInit 配成复用推挽 */
}

/* USER CODE BEGIN 4 */
/**
  * @brief 定时器更新中断回调（模式1）
  * @note  每 1 s 进一次，翻转一次 LED → 亮-灭周期 2 s
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3)
  {
    HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
  }
}
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1) { }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) { (void)file; (void)line; }
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
