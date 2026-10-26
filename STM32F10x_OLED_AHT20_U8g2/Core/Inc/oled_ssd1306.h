/*
 * oled_ssd1306.h - 自定义 SSD1306 128x64 I2C 驱动
 */
#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OLED_WIDTH       128U
#define OLED_HEIGHT      64U
#define OLED_PAGES       8U
#define OLED_BUF_SIZE    (OLED_WIDTH * OLED_PAGES)

/* 文本模式 */
#define OLED_TEXT_5X7      0U   /* 纯 ASCII，y 为字符顶部 */
#define OLED_TEXT_MIXED16  1U   /* 汉字 16x16，ASCII 在 16px 行内垂直居中 */

/* 基础控制 */
uint8_t OLED_Init(void);
uint8_t OLED_IsPresent(void);
void OLED_Clear(void);
void OLED_Display(void);

/* 绘图 */
void OLED_DrawPixel(int16_t x, int16_t y, uint8_t color);
void OLED_DrawHLine(int16_t x, int16_t y, int16_t w, uint8_t color);
void OLED_DrawVLine(int16_t x, int16_t y, int16_t h, uint8_t color);
void OLED_DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void OLED_DrawFrame(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void OLED_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
void OLED_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint8_t color);

/* 点阵：行优先，每行按字节对齐，MSB 在左 */
void OLED_DrawBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *bmp);

/* 字符与汉字 */
void OLED_DrawChar5x7(int16_t x, int16_t y, char c, uint8_t color);
void OLED_DrawChar5x7Scaled(int16_t x, int16_t y, char c, uint8_t scale, uint8_t color);
void OLED_DrawString5x7(int16_t x, int16_t y, const char *s, uint8_t color);
void OLED_DrawString5x7Scaled(int16_t x, int16_t y, const char *s, uint8_t scale, uint8_t color);
void OLED_DrawGlyph16(int16_t x, int16_t y, const uint8_t *glyph, uint8_t color);
void OLED_DrawCNChar16(int16_t x, int16_t y, uint16_t code, uint8_t color);
uint16_t OLED_TextWidthU16(const uint16_t *s);
void OLED_DrawTextU16(int16_t x, int16_t y, const uint16_t *s, uint8_t mode, uint8_t color);

/* SSD1306 硬件滚动 */
void OLED_ScrollStop(void);
void OLED_HScrollStart(uint8_t left, uint8_t start_page, uint8_t end_page, uint8_t interval);
void OLED_VScrollStart(uint8_t left, uint8_t start_page, uint8_t end_page, uint8_t interval, uint8_t v_offset);

#ifdef __cplusplus
}
#endif

#endif /* OLED_SSD1306_H */
