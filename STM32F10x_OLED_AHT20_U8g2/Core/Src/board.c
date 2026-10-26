/*
 * board.c - 板级服务
 */
#include "main.h"
#include "board.h"
#include "app_config.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static uint32_t s_led_tick;
static uint8_t  s_key_last;

void Board_Init(void)
{
  LED_OFF();
  s_led_tick = HAL_GetTick();
  s_key_last = 0;
}

/**
  * @brief 周期任务：LED 500ms 心跳
  */
void Board_Service(void)
{
  uint32_t now = HAL_GetTick();
  if ((uint32_t)(now - s_led_tick) >= 500U)
  {
    s_led_tick = now;
    LED_TOGGLE();
  }
}

/**
  * @brief 按键消抖：检测一次按下（下降沿），返回 1
  */
uint8_t Board_KeyPressed(void)
{
  uint8_t now = KEY_IS_DOWN() ? 1U : 0U;
  uint8_t pressed = 0;

  if (now && !s_key_last)
  {
    HAL_Delay(20);
    if (KEY_IS_DOWN())
    {
      pressed = 1;
    }
  }
  s_key_last = now;
  return pressed;
}

/**
  * @brief 带心跳服务的延时
  */
void Board_Delay(uint32_t ms)
{
  uint32_t start = HAL_GetTick();
  while ((uint32_t)(HAL_GetTick() - start) < ms)
  {
    Board_Service();
    HAL_Delay(5);
  }
}

void Board_Print(const char *s)
{
#if APP_DEBUG_UART
  if (s != 0)
  {
    HAL_UART_Transmit(&huart1, (uint8_t *)s, (uint16_t)strlen(s), 100);
  }
#else
  (void)s;
#endif
}

void Board_Printf(const char *fmt, ...)
{
#if APP_DEBUG_UART
  char buf[160];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  Board_Print(buf);
#else
  (void)fmt;
#endif
}
