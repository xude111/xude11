/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_hal_msp.c
  * @brief   MCU Support package —— 外设底层初始化（时钟 + 引脚 + 中断 + DMA）
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN 0 */
extern DMA_HandleTypeDef hdma_usart1_tx;
/* USER CODE END 0 */

/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /* 关闭 JTAG、保留 SWD：释放 PA15/PB3/PB4，同时保留 SWD 下载调试 */
  __HAL_AFIO_REMAP_SWJ_NOJTAG();
}

/**
  * @brief UART MSP Initialization
  * @note  由 HAL_UART_Init() 自动调用：
  *        ① 打开 USART1 / GPIOA 时钟
  *        ② PA9 复用推挽输出(TX)，PA10 浮空输入(RX)
  *        ③ 使能 USART1 中断（接收 '#''*' 用）
  *        ④ 配置 USART1_TX 的 DMA 通道（DMA1_Channel4）
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (huart->Instance == USART1)
  {
    /* USER CODE BEGIN USART1_MspInit 0 */

    /* USER CODE END USART1_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /**USART1 GPIO Configuration
    PA9      ------> USART1_TX
    PA10     ------> USART1_RX
    */
    /* PA9 发送：复用推挽输出 */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA10 接收：浮空输入 */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init（接收中断） */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    /* USER CODE BEGIN USART1_MspInit 1 */
    /* ---------------- USART1_TX DMA 初始化（任务二-3） ----------------
       STM32F103 的 USART1_TX 固定映射到 DMA1 的通道 4。
       方向：内存 → 外设；外设地址不自增（始终是 USART1->DR），
       内存地址自增（依次读缓冲区）；数据宽度都是 8 位；
       模式选【循环 DMA_CIRCULAR】—— 传输结束硬件自动重装，形成连续发送。 */
    hdma_usart1_tx.Instance = DMA1_Channel4;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_CIRCULAR;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    /* 把 DMA 句柄和串口句柄绑定，HAL_UART_Transmit_DMA 才能找到它 */
    __HAL_LINKDMA(huart, hdmatx, hdma_usart1_tx);
    /* USER CODE END USART1_MspInit 1 */
  }
}

/**
  * @brief UART MSP De-Initialization
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if (huart->Instance == USART1)
  {
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9      ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);

    /* USART1 interrupt DeInit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);

    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(huart->hdmatx);
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
