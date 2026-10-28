/*
*********************************************************************************************************
* 文件：app.c
* 功能：uC/OS-II 应用任务
*   任务 1：PA0 红灯，周期 1 s（亮 500 ms、灭 500 ms）
*   任务 2：PB0 绿灯，周期 3 s（亮 1500 ms、灭 1500 ms）
*   任务 3：每 2 s 通过 I2C1 读取 AHT20 温湿度，并从 USART1 发送
*
* 硬件：
*   PA0  -> 220Ω -> 红色 LED -> GND（高电平点亮）
*   PB0  -> 220Ω -> 绿色 LED -> GND（高电平点亮）
*   PB6  -> AHT20 SCL（I2C1，400 kHz）
*   PB7  -> AHT20 SDA
*   PA9  -> USB-TTL RXD
*   PA10 <- USB-TTL TXD
*   GND  <-> USB-TTL GND
*********************************************************************************************************
*/
#include "app.h"
#include "main.h"
#include "aht20.h"

extern UART_HandleTypeDef huart1;

/* ---------------------------- 任务堆栈 ---------------------------- */
static OS_STK App_TaskLed1Stk[APP_TASK_LED1_STK_SIZE];
static OS_STK App_TaskLed2Stk[APP_TASK_LED2_STK_SIZE];
static OS_STK App_TaskAht20Stk[APP_TASK_UART_STK_SIZE];

/* 串口发送缓冲：AHT20 温湿度字符串 */
static char g_msg[128];

/* ---------------------------- 任务 1：1 s 周期红灯 ---------------------------- */
static void App_TaskLed1(void *p_arg)
{
    (void)p_arg;

    for (;;)
    {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);   /* 点亮红灯 */
        OSTimeDlyHMSM(0u, 0u, 0u, 500u);                      /* 亮 500 ms */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET); /* 熄灭红灯 */
        OSTimeDlyHMSM(0u, 0u, 0u, 500u);                      /* 灭 500 ms */
    }
}

/* ---------------------------- 任务 2：3 s 周期绿灯 ---------------------------- */
static void App_TaskLed2(void *p_arg)
{
    (void)p_arg;

    for (;;)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);   /* 点亮绿灯 */
        OSTimeDlyHMSM(0u, 0u, 1u, 500u);                      /* 亮 1500 ms */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); /* 熄灭绿灯 */
        OSTimeDlyHMSM(0u, 0u, 1u, 500u);                      /* 灭 1500 ms */
    }
}

/* ---------------------------- 任务 3：2 s 周期 AHT20 采集 + 串口发送 ---------------------------- */
static void App_TaskAht20(void *p_arg)
{
    float temperature = 0.0f;
    float humidity = 0.0f;
    int len;

    (void)p_arg;

    if (AHT20_Init() != 0u)
    {
        const char *err = "AHT20 init failed, check PB6/PB7 and 3.3V\r\n";
        (void)HAL_UART_Transmit(&huart1, (uint8_t *)err,
                                (uint16_t)strlen(err), 1000u);
    }

    for (;;)
    {
        if (AHT20_Read(&temperature, &humidity) == 0u)
        {
            len = snprintf(g_msg, sizeof(g_msg),
                           "hello uc/OS! AHT20: T=%.2f C, H=%.2f %%RH\r\n",
                           temperature, humidity);
        }
        else
        {
            len = snprintf(g_msg, sizeof(g_msg),
                           "hello uc/OS! AHT20 read error\r\n");
        }

        if (len > 0)
        {
            (void)HAL_UART_Transmit(&huart1, (uint8_t *)g_msg,
                                    (uint16_t)len, 1000u);
        }
        OSTimeDlyHMSM(0u, 0u, 2u, 0u);                        /* 2 s 周期 */
    }
}

/* ---------------------------- 创建任务 ---------------------------- */
void App_Init(void)
{
    INT8U err;

    err = OSTaskCreate(App_TaskLed1,
                       (void *)0,
                       &App_TaskLed1Stk[APP_TASK_LED1_STK_SIZE - 1u],
                       APP_TASK_LED1_PRIO);
    if (err != OS_ERR_NONE) { Error_Handler(); }

    err = OSTaskCreate(App_TaskLed2,
                       (void *)0,
                       &App_TaskLed2Stk[APP_TASK_LED2_STK_SIZE - 1u],
                       APP_TASK_LED2_PRIO);
    if (err != OS_ERR_NONE) { Error_Handler(); }

    err = OSTaskCreate(App_TaskAht20,
                       (void *)0,
                       &App_TaskAht20Stk[APP_TASK_UART_STK_SIZE - 1u],
                       APP_TASK_UART_PRIO);
    if (err != OS_ERR_NONE) { Error_Handler(); }
}
