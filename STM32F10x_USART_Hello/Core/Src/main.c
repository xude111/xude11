/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : USART1 串口通信 —— 任务一 + 任务二（中断 / DMA）
  ******************************************************************************
  * 任务一   ：阻塞方式连续发送 "hello windows！"，上位机发 '#' 暂停、发 '*' 继续
  * 任务二-1 ：改用【串口中断方式】(HAL_UART_Transmit_IT) 重做上面的收发
  * 任务二-3 ：改用【串口 DMA 方式】(HAL_UART_Transmit_DMA，循环模式)
  *            以 115200bps 连续向 PC 发送数据，CPU 不参与搬运
  *
  * 本工程用宏 USART_MODE 切换三种实现，编译前改这一行即可：
  *     USART_MODE = 0  任务一    阻塞发送  HAL_UART_Transmit
  *     USART_MODE = 1  任务二-1  中断发送  HAL_UART_Transmit_IT
  *     USART_MODE = 2  任务二-3  DMA 发送  HAL_UART_Transmit_DMA（循环模式）
  *
  * 硬件连接：
  *     PA9  (USART1_TX) → USB-TTL 的 RXD
  *     PA10 (USART1_RX) ← USB-TTL 的 TXD
  *     GND              ↔ USB-TTL 的 GND
  * 串口参数：115200, 8 位数据, 1 位停止, 无校验, 无流控
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* ★ 选择实现方式：0 = 任务一(阻塞)  1 = 任务二-1(中断)  2 = 任务二-3(DMA) ★ */
#define USART_MODE        0U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
DMA_HandleTypeDef  hdma_usart1_tx;

/* USER CODE BEGIN PV */
/* 发送内容。末尾 ！ 是全角字符(UTF-8 占 3 字节)，串口助手编码要选 UTF-8。
   \r\n 只为在助手上分行显示，便于观察与计数。 */
static const uint8_t g_hello[] = "hello windows！\r\n";

/* 发送开关：1 = 发送，0 = 暂停（'#' 置 0，'*' 置 1） */
volatile uint8_t g_send_enable = 1U;

/* 串口接收缓冲：中断方式，每收 1 字节进一次回调 */
static uint8_t g_rx_byte = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  /* 串口接收一律用中断方式（三种模式都用它接 '#' 和 '*'） */
  if (HAL_UART_Receive_IT(&huart1, &g_rx_byte, 1U) != HAL_OK)
  {
    Error_Handler();
  }

#if (USART_MODE == 1U)
  /* 任务二-1：启动第一次中断发送，之后在 HAL_UART_TxCpltCallback 里自动续发 */
  if (HAL_UART_Transmit_IT(&huart1, (uint8_t *)g_hello,
                           (uint16_t)(sizeof(g_hello) - 1U)) != HAL_OK)
  {
    Error_Handler();
  }
#endif

#if (USART_MODE == 2U)
  /* 任务二-3：启动 DMA 发送。DMA 通道配置成【循环模式】，
     硬件会自动把同一缓冲区一遍遍发出去，CPU 完全不参与。 */
  if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *)g_hello,
                            (uint16_t)(sizeof(g_hello) - 1U)) != HAL_OK)
  {
    Error_Handler();
  }
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
#if (USART_MODE == 0U)
    /* ================= 任务一：阻塞方式 ================= */
    if (g_send_enable != 0U)
    {
      (void)HAL_UART_Transmit(&huart1, (uint8_t *)g_hello,
                              (uint16_t)(sizeof(g_hello) - 1U), 100U);
      /* 任务一要求：两次发送之间【不加延时】。这里确实没有任何延时，
         用来观察上位机是否因为处理不过来而丢数据。 */
    }
#elif (USART_MODE == 1U)
    /* ================= 任务二-1：中断方式 =================
       发送完全由中断驱动：每次发完 HAL 自动回调 TxCplt，
       在回调里再挂一次 Transmit_IT，形成"连续发送"。
       主循环这里空转即可（CPU 被释放出来做别的事）。 */
    /* nothing to do here */
#else
    /* ================= 任务二-3：DMA 方式 =================
       DMA 工作在循环模式，硬件自动重复发送，主循环同样空转。 */
    /* nothing to do here */
#endif
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* USER CODE END 3 */
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
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

  /** Initializes the CPU, AHB and APB buses clocks
  */
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
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;                    /* 波特率 115200      */
  huart1.Init.WordLength = UART_WORDLENGTH_8B;      /* 8 位数据位          */
  huart1.Init.StopBits = UART_STOPBITS_1;           /* 1 位停止位          */
  huart1.Init.Parity = UART_PARITY_NONE;            /* 无校验位            */
  huart1.Init.Mode = UART_MODE_TX_RX;               /* 允许发送 + 接收     */
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;      /* 无硬件流控          */
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Enable DMA controller clock
  * @retval None
  */
static void MX_DMA_Init(void)
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel4_IRQn interrupt configuration (USART1_TX) */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USART1 的 PA9/PA10 在 HAL_UART_MspInit() 里配置 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
  * @brief  串口【发送完成】回调 —— 任务二-1 的"连续发送"就靠它
  * @note   中断方式每次只发一串，发完进这里；
  *         在这里再挂一次 Transmit_IT，就形成了不间断的连续发送。
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
#if (USART_MODE == 1U)
    if (g_send_enable != 0U)
    {
      (void)HAL_UART_Transmit_IT(&huart1, (uint8_t *)g_hello,
                                 (uint16_t)(sizeof(g_hello) - 1U));
    }
#endif
  }
}

/**
  * @brief  串口【接收完成】回调 —— 收到 '#' 暂停、'*' 继续
  * @note   三种模式共用这一套控制逻辑。
  *         DMA 模式下暂停/恢复要动 DMA 通道，所以单独处理。
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    if (g_rx_byte == (uint8_t)'#')
    {
      g_send_enable = 0U;                       /* 暂停发送 */
#if (USART_MODE == 2U)
      (void)HAL_UART_DMAStop(&huart1);          /* DMA 模式：停掉 DMA 传输 */
#endif
    }
    else if (g_rx_byte == (uint8_t)'*')
    {
      g_send_enable = 1U;                       /* 继续发送 */
#if (USART_MODE == 2U)
      (void)HAL_UART_Transmit_DMA(&huart1, (uint8_t *)g_hello,
                                  (uint16_t)(sizeof(g_hello) - 1U));
#elif (USART_MODE == 1U)
      (void)HAL_UART_Transmit_IT(&huart1, (uint8_t *)g_hello,
                                 (uint16_t)(sizeof(g_hello) - 1U));
#endif
    }

    /* 必须重新开启接收，否则只能收到第一个字符 */
    (void)HAL_UART_Receive_IT(&huart1, &g_rx_byte, 1U);
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  (void)file;
  (void)line;
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
