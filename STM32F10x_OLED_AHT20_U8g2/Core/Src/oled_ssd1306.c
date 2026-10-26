/*
 * oled_ssd1306.c - 自定义 SSD1306 128x64 I2C 驱动
 *
 * 采用 1KB 显存（8 页 x 128 列），所有绘图先写显存，
 * 再通过 OLED_Display() 一次性刷新，避免 I2C 频繁小包传输。
 */
#include "main.h"
#include "oled_ssd1306.h"
#include "font5x7.h"
#include "font_cn.h"
#include "app_config.h"
#include <string.h>

static uint8_t s_gram[OLED_BUF_SIZE];
static uint8_t s_txbuf[OLED_WIDTH + 1];

/* ---------------- 底层 I2C ---------------- */

static void OLED_WriteCmd(uint8_t cmd)
{
  s_txbuf[0] = 0x00U;
  s_txbuf[1] = cmd;
  HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, s_txbuf, 2, 100);
}

static void OLED_WriteCmds(const uint8_t *cmds, uint16_t len)
{
  uint16_t i;
  if (len > OLED_WIDTH)
  {
    len = OLED_WIDTH;
  }
  s_txbuf[0] = 0x00U;
  for (i = 0; i < len; i++)
  {
    s_txbuf[i + 1] = cmds[i];
  }
  HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, s_txbuf, (uint16_t)(len + 1), 100);
}

static void OLED_WriteData(const uint8_t *data, uint16_t len)
{
  uint16_t i;
  if (len > OLED_WIDTH)
  {
    len = OLED_WIDTH;
  }
  s_txbuf[0] = 0x40U;
  for (i = 0; i < len; i++)
  {
    s_txbuf[i + 1] = data[i];
  }
  HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, s_txbuf, (uint16_t)(len + 1), 100);
}

static void OLED_SetCursor(uint8_t page, uint8_t col)
{
  OLED_WriteCmd((uint8_t)(0xB0U + page));
  OLED_WriteCmd((uint8_t)(0x00U + (col & 0x0FU)));
  OLED_WriteCmd((uint8_t)(0x10U + ((col >> 4) & 0x0FU)));
}

/* ---------------- 初始化与刷新 ---------------- */

uint8_t OLED_IsPresent(void)
{
  return (HAL_I2C_IsDeviceReady(&hi2c1, OLED_I2C_ADDR, 3, 100) == HAL_OK) ? 1U : 0U;
}

uint8_t OLED_Init(void)
{
  static const uint8_t init_seq[] =
  {
    0xAEU,              /* 关闭显示 */
    0xD5U, 0x80U,       /* 时钟分频 */
    0xA8U, 0x3FU,       /* 复用率 1/64 */
    0xD3U, 0x00U,       /* 显示偏移 0 */
    0x40U,              /* 起始行 0 */
    0x8DU, 0x14U,       /* 电荷泵开 */
    0x20U, 0x00U,       /* 水平寻址 */
    0xA1U,              /* 段重映射 */
    0xC8U,              /* COM 扫描方向 */
    0xDAU, 0x12U,       /* COM 引脚配置 */
    0x81U, 0xCFU,       /* 对比度 */
    0xD9U, 0xF1U,       /* 预充电周期 */
    0xDBU, 0x40U,       /* VCOMH */
    0xA4U,              /* 显示显存内容 */
    0xA6U,              /* 正常显示 */
    0x2EU,              /* 停止滚动 */
    0xAFU               /* 开显示 */
  };

  if (!OLED_IsPresent())
  {
    return 1;
  }

  OLED_WriteCmds(init_seq, sizeof(init_seq));
  OLED_Clear();
  OLED_Display();
  return 0;
}

void OLED_Clear(void)
{
  memset(s_gram, 0, sizeof(s_gram));
}

void OLED_Display(void)
{
  uint8_t page;
  OLED_SetCursor(0, 0);
  for (page = 0; page < OLED_PAGES; page++)
  {
    OLED_WriteData(&s_gram[page * OLED_WIDTH], OLED_WIDTH);
  }
}

/* ---------------- 基础绘图 ---------------- */

void OLED_DrawPixel(int16_t x, int16_t y, uint8_t color)
{
  if (x < 0 || x >= (int16_t)OLED_WIDTH || y < 0 || y >= (int16_t)OLED_HEIGHT)
  {
    return;
  }
  if (color)
  {
    s_gram[(y >> 3) * OLED_WIDTH + x] |= (uint8_t)(1U << (y & 7));
  }
  else
  {
    s_gram[(y >> 3) * OLED_WIDTH + x] &= (uint8_t)~(1U << (y & 7));
  }
}

void OLED_DrawHLine(int16_t x, int16_t y, int16_t w, uint8_t color)
{
  int16_t i;
  for (i = 0; i < w; i++)
  {
    OLED_DrawPixel((int16_t)(x + i), y, color);
  }
}

void OLED_DrawVLine(int16_t x, int16_t y, int16_t h, uint8_t color)
{
  int16_t i;
  for (i = 0; i < h; i++)
  {
    OLED_DrawPixel(x, (int16_t)(y + i), color);
  }
}

void OLED_DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
  int16_t i, j;
  for (j = 0; j < h; j++)
  {
    for (i = 0; i < w; i++)
    {
      OLED_DrawPixel((int16_t)(x + i), (int16_t)(y + j), color);
    }
  }
}

void OLED_DrawFrame(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
  OLED_DrawHLine(x, y, w, color);
  OLED_DrawHLine(x, (int16_t)(y + h - 1), w, color);
  OLED_DrawVLine(x, y, h, color);
  OLED_DrawVLine((int16_t)(x + w - 1), y, h, color);
}

void OLED_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
{
  int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
  int16_t sx = (x0 < x1) ? 1 : -1;
  int16_t dy = (y1 > y0) ? -(y1 - y0) : -(y0 - y1);
  int16_t sy = (y0 < y1) ? 1 : -1;
  int16_t err = (int16_t)(dx + dy);

  while (1)
  {
    OLED_DrawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1)
    {
      break;
    }
    {
      int16_t e2 = (int16_t)(2 * err);
      if (e2 >= dy)
      {
        err = (int16_t)(err + dy);
        x0 = (int16_t)(x0 + sx);
      }
      if (e2 <= dx)
      {
        err = (int16_t)(err + dx);
        y0 = (int16_t)(y0 + sy);
      }
    }
  }
}

void OLED_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color)
{
  int16_t x = 0;
  int16_t y = r;
  int16_t d = (int16_t)(3 - 2 * r);

  while (y >= x)
  {
    OLED_DrawPixel((int16_t)(x0 + x), (int16_t)(y0 + y), color);
    OLED_DrawPixel((int16_t)(x0 - x), (int16_t)(y0 + y), color);
    OLED_DrawPixel((int16_t)(x0 + x), (int16_t)(y0 - y), color);
    OLED_DrawPixel((int16_t)(x0 - x), (int16_t)(y0 - y), color);
    OLED_DrawPixel((int16_t)(x0 + y), (int16_t)(y0 + x), color);
    OLED_DrawPixel((int16_t)(x0 - y), (int16_t)(y0 + x), color);
    OLED_DrawPixel((int16_t)(x0 + y), (int16_t)(y0 - x), color);
    OLED_DrawPixel((int16_t)(x0 - y), (int16_t)(y0 - x), color);

    if (d < 0)
    {
      d = (int16_t)(d + 4 * x + 6);
    }
    else
    {
      d = (int16_t)(d + 4 * (x - y) + 10);
      y--;
    }
    x++;
  }
}

void OLED_DrawBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *bmp)
{
  int16_t row, col;
  int16_t bytes_per_row;
  if (bmp == 0 || w <= 0 || h <= 0)
  {
    return;
  }
  bytes_per_row = (int16_t)((w + 7) / 8);
  for (row = 0; row < h; row++)
  {
    for (col = 0; col < w; col++)
    {
      uint8_t byte = bmp[row * bytes_per_row + (col >> 3)];
      if (byte & (uint8_t)(0x80U >> (col & 7)))
      {
        OLED_DrawPixel((int16_t)(x + col), (int16_t)(y + row), 1);
      }
    }
  }
}

/* ---------------- 字符与汉字 ---------------- */

void OLED_DrawChar5x7(int16_t x, int16_t y, char c, uint8_t color)
{
  const uint8_t *font = Font5x7_Get(c);
  int16_t col, row;
  for (col = 0; col < 5; col++)
  {
    uint8_t bits = font[col];
    for (row = 0; row < 7; row++)
    {
      if (bits & (uint8_t)(1U << row))
      {
        OLED_DrawPixel((int16_t)(x + col), (int16_t)(y + row), color);
      }
    }
  }
}

void OLED_DrawChar5x7Scaled(int16_t x, int16_t y, char c, uint8_t scale, uint8_t color)
{
  const uint8_t *font = Font5x7_Get(c);
  int16_t col, row, dx, dy;
  if (scale == 0)
  {
    scale = 1;
  }
  for (col = 0; col < 5; col++)
  {
    uint8_t bits = font[col];
    for (row = 0; row < 7; row++)
    {
      if (bits & (uint8_t)(1U << row))
      {
        for (dy = 0; dy < scale; dy++)
        {
          for (dx = 0; dx < scale; dx++)
          {
            OLED_DrawPixel((int16_t)(x + col * scale + dx),
                           (int16_t)(y + row * scale + dy), color);
          }
        }
      }
    }
  }
}

void OLED_DrawString5x7(int16_t x, int16_t y, const char *s, uint8_t color)
{
  while (*s)
  {
    OLED_DrawChar5x7(x, y, *s, color);
    x = (int16_t)(x + 6);
    s++;
  }
}

void OLED_DrawString5x7Scaled(int16_t x, int16_t y, const char *s, uint8_t scale, uint8_t color)
{
  if (scale == 0)
  {
    scale = 1;
  }
  while (*s)
  {
    OLED_DrawChar5x7Scaled(x, y, *s, scale, color);
    x = (int16_t)(x + 6 * scale);
    s++;
  }
}

void OLED_DrawGlyph16(int16_t x, int16_t y, const uint8_t *glyph, uint8_t color)
{
  int16_t row, col;
  if (glyph == 0)
  {
    return;
  }
  for (row = 0; row < 16; row++)
  {
    uint8_t b1 = glyph[row * 2];
    uint8_t b2 = glyph[row * 2 + 1];
    for (col = 0; col < 8; col++)
    {
      if (b1 & (uint8_t)(0x80U >> col))
      {
        OLED_DrawPixel((int16_t)(x + col), (int16_t)(y + row), color);
      }
      if (b2 & (uint8_t)(0x80U >> col))
      {
        OLED_DrawPixel((int16_t)(x + 8 + col), (int16_t)(y + row), color);
      }
    }
  }
}

void OLED_DrawCNChar16(int16_t x, int16_t y, uint16_t code, uint8_t color)
{
  if (code < 0x80U)
  {
    OLED_DrawChar5x7((int16_t)(x + 5), (int16_t)(y + 5), (char)code, color);
  }
  else
  {
    const uint8_t *glyph = FontCN_Get(code);
    if (glyph != 0)
    {
      OLED_DrawGlyph16(x, y, glyph, color);
    }
    else
    {
      /* 缺字：画空心方框 + 问号 */
      OLED_DrawFrame(x, y, 16, 16, color);
      OLED_DrawChar5x7((int16_t)(x + 5), (int16_t)(y + 5), '?', color);
    }
  }
}

uint16_t OLED_TextWidthU16(const uint16_t *s)
{
  uint16_t w = 0;
  if (s == 0)
  {
    return 0;
  }
  while (*s)
  {
    if (*s < 0x80U)
    {
      w = (uint16_t)(w + 6);
    }
    else
    {
      w = (uint16_t)(w + 16);
    }
    s++;
  }
  return w;
}

void OLED_DrawTextU16(int16_t x, int16_t y, const uint16_t *s, uint8_t mode, uint8_t color)
{
  if (s == 0)
  {
    return;
  }
  while (*s)
  {
    uint16_t code = *s;
    if (code < 0x80U)
    {
      int16_t ay = (mode == OLED_TEXT_MIXED16) ? (int16_t)(y + 5) : y;
      OLED_DrawChar5x7(x, ay, (char)code, color);
      x = (int16_t)(x + 6);
    }
    else
    {
      OLED_DrawCNChar16(x, y, code, color);
      x = (int16_t)(x + 16);
    }
    s++;
  }
}

/* ---------------- 硬件滚动 ---------------- */

void OLED_ScrollStop(void)
{
  OLED_WriteCmd(0x2EU);
}

void OLED_HScrollStart(uint8_t left, uint8_t start_page, uint8_t end_page, uint8_t interval)
{
  uint8_t cmds[7];
  cmds[0] = left ? 0x27U : 0x26U;  /* 0x27=左滚, 0x26=右滚 */
  cmds[1] = 0x00U;
  cmds[2] = (uint8_t)(start_page & 0x07U);
  cmds[3] = (uint8_t)(interval & 0x07U);
  cmds[4] = (uint8_t)(end_page & 0x07U);
  cmds[5] = 0x00U;
  cmds[6] = 0xFFU;
  OLED_WriteCmds(cmds, 7);
  OLED_WriteCmd(0x2FU);
}

void OLED_VScrollStart(uint8_t left, uint8_t start_page, uint8_t end_page,
                       uint8_t interval, uint8_t v_offset)
{
  uint8_t cmds[7];
  cmds[0] = left ? 0x2AU : 0x29U;  /* 0x2A=左+垂直, 0x29=右+垂直 */
  cmds[1] = 0x00U;
  cmds[2] = (uint8_t)(start_page & 0x07U);
  cmds[3] = (uint8_t)(interval & 0x07U);
  cmds[4] = (uint8_t)(end_page & 0x07U);
  cmds[5] = (uint8_t)(v_offset & 0x3FU);
  cmds[6] = 0x00U;
  OLED_WriteCmds(cmds, 7);
  OLED_WriteCmd(0x2FU);
}
