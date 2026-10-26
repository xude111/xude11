/*
 * task3_app.c - 任务 3：移植裁剪版 U8g2 + demo + 学号姓名 + 四向滚动 + 动画头像
 *
 * U8g2 配置：SSD1306 128x64，I2C1（PB6/PB7，400kHz，地址 0x78）
 * 页面流程：
 *   1. U8g2 图形/字体 demo（含动画）
 *   2. 学号姓名页
 *   3. 四向滚动页（左、右、上、下）
 *   4. 动画头像页
 *   5. AHT20 温湿度页
 */
#include "main.h"
#include "task3_app.h"
#include "app_config.h"
#include "board.h"
#include "aht20.h"
#include "font_cn.h"
#include "font5x7.h"
#include "avatar.h"
#include "u8g2.h"
#include <stdio.h>
#include <string.h>

#define U8G2_TX_BUF_SIZE    160U

static u8g2_t s_u8g2;
static uint8_t s_tx_buf[U8G2_TX_BUF_SIZE];
static uint16_t s_tx_len;

/* ---------------- 界面文字（Unicode 码点） ---------------- */

static const uint16_t TXT_NAME[] =
{
    0x5F90, 0x6B63, 0x6615, 0   /* 徐正昕 */
};

static const uint16_t TXT_CLASS[] =
{
    0x901A, 0x4FE1, '2', '4', '0', '3', 0   /* 通信2403 */
};

static const uint16_t TXT_SENSOR[] =
{
    0x6E29, 0x6E7F, 0x5EA6, 0x68C0, 0x6D4B, 0   /* 温湿度检测 */
};

static const uint16_t TXT_SCROLL[] =
{
    0x56DB, 0x5411, 0x6EDA, 0x52A8, 0x6F14, 0x793A, 0   /* 四向滚动演示 */
};

static const uint16_t TXT_AVATAR[] =
{
    0x5934, 0x50CF, 0   /* 头像 */
};

/* ---------------- U8g2 移植层：I2C 字节回调 ---------------- */

static uint8_t U8g2_ByteCb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    uint8_t *p;

    (void)u8x8;

    switch (msg)
    {
        case U8X8_MSG_BYTE_INIT:
            return 1;

        case U8X8_MSG_BYTE_START_TRANSFER:
            s_tx_len = 0;
            return 1;

        case U8X8_MSG_BYTE_SEND:
            p = (uint8_t *)arg_ptr;
            while ((arg_int > 0U) && (s_tx_len < (U8G2_TX_BUF_SIZE - 1U)))
            {
                s_tx_buf[s_tx_len++] = *p++;
                arg_int--;
            }
            return 1;

        case U8X8_MSG_BYTE_END_TRANSFER:
            if (s_tx_len > 0U)
            {
                HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, s_tx_buf, s_tx_len, 100);
            }
            return 1;

        case U8X8_MSG_BYTE_SET_DC:
            return 1;

        default:
            return 0;
    }
}

static uint8_t U8g2_GpioDelayCb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    volatile uint32_t n;

    (void)u8x8;
    (void)arg_ptr;

    switch (msg)
    {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
        case U8X8_MSG_GPIO_RESET:
        case U8X8_MSG_GPIO_I2C_CLOCK:
        case U8X8_MSG_GPIO_I2C_DATA:
            return 1;

        case U8X8_MSG_DELAY_MILLI:
            HAL_Delay(arg_int);
            return 1;

        case U8X8_MSG_DELAY_10MICRO:
            n = 180U;
            while (n--) { }
            return 1;

        case U8X8_MSG_DELAY_100NANO:
        case U8X8_MSG_DELAY_NANO:
        case U8X8_MSG_DELAY_I2C:
            return 1;

        default:
            return 0;
    }
}

static void U8g2_Init(void)
{
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&s_u8g2, U8G2_R0,
                                           U8g2_ByteCb, U8g2_GpioDelayCb);
    u8g2_InitDisplay(&s_u8g2);
    u8g2_SetPowerSave(&s_u8g2, 0);
    u8g2_ClearBuffer(&s_u8g2);
    u8g2_SendBuffer(&s_u8g2);
}

/* ---------------- 汉字/ASCII 混合绘制 ---------------- */

/**
  * @brief 把 5x7 ASCII 字符扩展成 16x16 XBMP 单元（垂直居中）
  */
static void T3_AsciiCell(uint8_t *cell, char c)
{
    const uint8_t *font = Font5x7_Get(c);
    uint8_t col, row;

    memset(cell, 0, 32);
    for (col = 0; col < 5; col++)
    {
        uint8_t bits = font[col];
        for (row = 0; row < 7; row++)
        {
            if (bits & (uint8_t)(1U << row))
            {
                uint8_t px = col;
                uint8_t py = (uint8_t)(row + 5U);
                cell[py * 2U + (px >> 3)] |= (uint8_t)(0x80U >> (px & 7U));
            }
        }
    }
}

/**
  * @brief 绘制 16x16 单元，支持被视口左/上边界裁剪
  */
static void T3_DrawCellClip(u8g2_t *u8g2, int16_t x, int16_t y,
                            const uint8_t *cell,
                            int16_t cx0, int16_t cy0, int16_t cx1, int16_t cy1)
{
    uint8_t tmp[32];
    int16_t skip_x = 0;
    int16_t skip_y = 0;
    int16_t row;

    if ((x >= cx1) || (y >= cy1) || ((x + 16) <= cx0) || ((y + 16) <= cy0))
    {
        return;
    }

    if ((x >= cx0) && (y >= cy0))
    {
        u8g2_DrawXBMP(u8g2, (u8g2_uint_t)x, (u8g2_uint_t)y, 16, 16, cell);
        return;
    }

    if (x < cx0)
    {
        skip_x = (int16_t)(cx0 - x);
    }
    if (y < cy0)
    {
        skip_y = (int16_t)(cy0 - y);
    }

    memset(tmp, 0, sizeof(tmp));
    for (row = skip_y; row < 16; row++)
    {
        uint16_t bits = ((uint16_t)cell[row * 2] << 8) | cell[row * 2 + 1];
        bits = (uint16_t)(bits << skip_x);
        tmp[(row - skip_y) * 2] = (uint8_t)(bits >> 8);
        tmp[(row - skip_y) * 2 + 1] = (uint8_t)(bits & 0xFFU);
    }

    u8g2_DrawXBMP(u8g2, (u8g2_uint_t)(x + skip_x), (u8g2_uint_t)(y + skip_y),
                  16, 16, tmp);
}

static const uint8_t *T3_GetGlyph(uint16_t code)
{
    static const uint8_t missing[32] =
    {
        0xFF, 0xFF, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
        0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
        0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
        0x80, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00
    };
    const uint8_t *g = FontCN_Get(code);
    return (g != 0) ? g : missing;
}

static uint16_t T3_TextWidth16(const uint16_t *s)
{
    uint16_t w = 0;
    while (*s)
    {
        w = (uint16_t)(w + ((*s < 0x80U) ? 6U : 16U));
        s++;
    }
    return w;
}

static void T3_DrawText16Clip(u8g2_t *u8g2, int16_t x, int16_t y,
                              const uint16_t *s,
                              int16_t cx0, int16_t cy0, int16_t cx1, int16_t cy1)
{
    uint8_t cell[32];

    while (*s)
    {
        if (*s < 0x80U)
        {
            T3_AsciiCell(cell, (char)*s);
            T3_DrawCellClip(u8g2, x, y, cell, cx0, cy0, cx1, cy1);
            x = (int16_t)(x + 6);
        }
        else
        {
            T3_DrawCellClip(u8g2, x, y, T3_GetGlyph(*s), cx0, cy0, cx1, cy1);
            x = (int16_t)(x + 16);
        }
        s++;
    }
}

static void T3_DrawText16(u8g2_t *u8g2, int16_t x, int16_t y, const uint16_t *s)
{
    T3_DrawText16Clip(u8g2, x, y, s, 0, 0, 128, 64);
}

/* ---------------- 通用等待 ---------------- */

static uint8_t Task3_Wait(uint32_t ms)
{
    uint32_t start = HAL_GetTick();
    while ((uint32_t)(HAL_GetTick() - start) < ms)
    {
        Board_Service();
        if (Board_KeyPressed())
        {
            return 1;
        }
        HAL_Delay(20);
    }
    return 0;
}

/* ---------------- 页面 1：U8g2 demo ---------------- */

static const int8_t s_sin16[16] =
{
    0, 6, 11, 15, 16, 15, 11, 6, 0, -6, -11, -15, -16, -15, -11, -6
};

static const int8_t s_cos16[16] =
{
    16, 15, 11, 6, 0, -6, -11, -15, -16, -15, -11, -6, 0, 6, 11, 15
};

static void Task3_DrawDemoPage(uint32_t elapsed)
{
    uint8_t idx = (uint8_t)((elapsed / 140U) & 0x0FU);
    int16_t cx = 32;
    int16_t cy = 40;
    int16_t px = (int16_t)(cx + (s_sin16[idx] * 13) / 16);
    int16_t py = (int16_t)(cy + (s_cos16[idx] * 13) / 16);
    uint8_t bar = (uint8_t)((elapsed / 80U) % 56U);

    u8g2_ClearBuffer(&s_u8g2);

    u8g2_SetFont(&s_u8g2, u8g2_font_7x14B_tr);
    u8g2_DrawStr(&s_u8g2, 4, 13, "U8g2 Demo");

    u8g2_SetFont(&s_u8g2, u8g2_font_5x7_tr);
    u8g2_DrawStr(&s_u8g2, 92, 12, "128x64");
    u8g2_DrawHLine(&s_u8g2, 0, 16, 128);

    /* 左侧：U8g2 图形函数与动画指针 */
    u8g2_DrawFrame(&s_u8g2, 2, 20, 60, 40);
    u8g2_DrawCircle(&s_u8g2, cx, cy, 16, U8G2_DRAW_ALL);
    u8g2_DrawLine(&s_u8g2, cx, cy, px, py);
    u8g2_DrawDisc(&s_u8g2, px, py, 3, U8G2_DRAW_ALL);
    u8g2_DrawDisc(&s_u8g2, cx, cy, 2, U8G2_DRAW_ALL);

    /* 右侧：内置字体演示 */
    u8g2_DrawBox(&s_u8g2, 68, 20, (u8g2_uint_t)(bar + 1U), 4);
    u8g2_SetFont(&s_u8g2, u8g2_font_ncenB14_tr);
    u8g2_DrawStr(&s_u8g2, 68, 42, "U8g2");
    u8g2_SetFont(&s_u8g2, u8g2_font_5x7_tr);
    u8g2_DrawStr(&s_u8g2, 68, 53, "STM32F103");
    u8g2_DrawStr(&s_u8g2, 68, 62, "SSD1306");

    u8g2_SendBuffer(&s_u8g2);
}

static void Task3_DemoPage(void)
{
    uint32_t start = HAL_GetTick();

    while ((uint32_t)(HAL_GetTick() - start) < PAGE_DEMO_MS)
    {
        Task3_DrawDemoPage((uint32_t)(HAL_GetTick() - start));
        Board_Service();
        if (Board_KeyPressed())
        {
            return;
        }
        HAL_Delay(50);
    }
}

/* ---------------- 页面 2：学号姓名 ---------------- */

static void Task3_DrawInfoPage(void)
{
    const char *title = "U8g2 PORT";
    u8g2_uint_t w;

    u8g2_ClearBuffer(&s_u8g2);

    u8g2_SetFont(&s_u8g2, u8g2_font_7x14B_tr);
    w = u8g2_GetStrWidth(&s_u8g2, title);
    u8g2_DrawStr(&s_u8g2, (u8g2_uint_t)((128U - w) / 2U), 13, title);
    u8g2_DrawHLine(&s_u8g2, 0, 16, 128);

    T3_DrawText16(&s_u8g2, 40, 20, TXT_NAME);

    u8g2_SetFont(&s_u8g2, u8g2_font_6x12_tr);
    u8g2_DrawStr(&s_u8g2, 28, 45, STUDENT_ID_STR);

    T3_DrawText16(&s_u8g2, 36, 48, TXT_CLASS);

    u8g2_SendBuffer(&s_u8g2);
}

static void Task3_InfoPage(void)
{
    Task3_DrawInfoPage();
    (void)Task3_Wait(PAGE_INFO_MS);
}

/* ---------------- 页面 3：四向滚动 ---------------- */

static void Task3_DrawScrollFrame(uint32_t elapsed)
{
    uint32_t phase = elapsed / 3000U;
    uint32_t t = elapsed % 3000U;
    uint16_t text_w = T3_TextWidth16(TXT_SCROLL);
    int16_t x = 0;
    int16_t top = 0;
    uint8_t i;

    u8g2_ClearBuffer(&s_u8g2);

    u8g2_SetFont(&s_u8g2, u8g2_font_5x7_tr);
    u8g2_DrawStr(&s_u8g2, 28, 8, "4-DIR SCROLL");
    u8g2_DrawHLine(&s_u8g2, 0, 10, 128);

    u8g2_SetClipWindow(&s_u8g2, 0, 11, 128, 55);

    switch (phase & 0x03U)
    {
        case 0:   /* 从右向左 */
            x = (int16_t)(128 - (int32_t)(t * 70U / 1000U));
            T3_DrawText16Clip(&s_u8g2, x, 27, TXT_SCROLL, 0, 11, 128, 55);
            break;

        case 1:   /* 从左向右 */
            x = (int16_t)(-(int32_t)text_w + (int32_t)(t * 70U / 1000U));
            T3_DrawText16Clip(&s_u8g2, x, 27, TXT_SCROLL, 0, 11, 128, 55);
            break;

        case 2:   /* 从下向上：竖向排列 */
            top = (int16_t)(64 - (int32_t)(t * 60U / 1000U));
            for (i = 0; i < 6U; i++)
            {
                T3_DrawCellClip(&s_u8g2, 56, (int16_t)(top + i * 18),
                                T3_GetGlyph(TXT_SCROLL[i]), 0, 11, 128, 55);
            }
            break;

        default:  /* 从上向下：竖向排列 */
            top = (int16_t)(-108 + (int32_t)(t * 60U / 1000U));
            for (i = 0; i < 6U; i++)
            {
                T3_DrawCellClip(&s_u8g2, 56, (int16_t)(top + i * 18),
                                T3_GetGlyph(TXT_SCROLL[i]), 0, 11, 128, 55);
            }
            break;
    }

    u8g2_SetMaxClipWindow(&s_u8g2);

    /* 底部方向指示 */
    u8g2_SetFont(&s_u8g2, u8g2_font_5x7_tr);
    if ((phase & 0x03U) == 0U)      { u8g2_DrawStr(&s_u8g2, 56, 62, "LEFT"); }
    else if ((phase & 0x03U) == 1U) { u8g2_DrawStr(&s_u8g2, 53, 62, "RIGHT"); }
    else if ((phase & 0x03U) == 2U) { u8g2_DrawStr(&s_u8g2, 59, 62, "UP"); }
    else                            { u8g2_DrawStr(&s_u8g2, 56, 62, "DOWN"); }

    u8g2_SendBuffer(&s_u8g2);
}

static void Task3_ScrollPage(void)
{
    uint32_t start = HAL_GetTick();

    while ((uint32_t)(HAL_GetTick() - start) < 12000U)
    {
        Task3_DrawScrollFrame((uint32_t)(HAL_GetTick() - start));
        Board_Service();
        if (Board_KeyPressed())
        {
            return;
        }
        HAL_Delay(25);
    }
}

/* ---------------- 页面 4：动画头像 ---------------- */

static void Task3_DrawAvatarPage(uint8_t frame)
{
    const char *id = STUDENT_ID_STR;

    u8g2_ClearBuffer(&s_u8g2);

    u8g2_SetFont(&s_u8g2, u8g2_font_5x7_tr);
    u8g2_DrawStr(&s_u8g2, 28, 6, id);

    u8g2_DrawXBMP(&s_u8g2, 2, 7, 56, 56,
                  (frame != 0U) ? avatar_frame1 : avatar_frame0);

    T3_DrawText16(&s_u8g2, 80, 12, TXT_AVATAR);   /* 头像 */
    T3_DrawText16(&s_u8g2, 74, 32, TXT_NAME);     /* 徐正昕 */
    T3_DrawText16(&s_u8g2, 70, 48, TXT_CLASS);    /* 通信2403 */

    u8g2_SendBuffer(&s_u8g2);
}

static void Task3_AvatarPage(void)
{
    uint32_t start = HAL_GetTick();

    while ((uint32_t)(HAL_GetTick() - start) < PAGE_AVATAR_MS)
    {
        uint32_t elapsed = (uint32_t)(HAL_GetTick() - start);
        uint32_t cyc = elapsed % 2000U;
        uint8_t frame = (cyc > 1550U) ? 1U : 0U;

        Task3_DrawAvatarPage(frame);
        Board_Service();
        if (Board_KeyPressed())
        {
            return;
        }
        HAL_Delay(80);
    }
}

/* ---------------- 页面 5：AHT20 温湿度 ---------------- */

static void Task3_FormatTemp(char *buf, float t)
{
    int32_t v = (int32_t)(t * 10.0f);
    if (v < 0)
    {
        v = -v;
        sprintf(buf, "-%d.%d C", (int)(v / 10), (int)(v % 10));
    }
    else
    {
        sprintf(buf, "%d.%d C", (int)(v / 10), (int)(v % 10));
    }
}

static void Task3_FormatHumi(char *buf, float h)
{
    int32_t v = (int32_t)(h * 10.0f);
    if (v < 0)
    {
        v = 0;
    }
    sprintf(buf, "%d.%d %%", (int)(v / 10), (int)(v % 10));
}

static void Task3_DrawSensorPage(float t, float h, uint8_t valid)
{
    char tbuf[16];
    char hbuf[16];
    char status[20];

    if (valid)
    {
        Task3_FormatTemp(tbuf, t);
        Task3_FormatHumi(hbuf, h);
        sprintf(status, "AHT20 OK");
    }
    else
    {
        sprintf(tbuf, "--.- C");
        sprintf(hbuf, "--.- %%");
        sprintf(status, "AHT20 ERR");
    }

    u8g2_ClearBuffer(&s_u8g2);

    T3_DrawText16(&s_u8g2, 24, 0, TXT_SENSOR);
    u8g2_DrawHLine(&s_u8g2, 0, 17, 128);

    u8g2_DrawXBMP(&s_u8g2, 6, 22, 16, 16, icon_temp);
    u8g2_SetFont(&s_u8g2, u8g2_font_7x14B_tr);
    u8g2_DrawStr(&s_u8g2, 28, 35, tbuf);

    u8g2_DrawXBMP(&s_u8g2, 6, 39, 16, 16, icon_humi);
    u8g2_DrawStr(&s_u8g2, 28, 52, hbuf);

    u8g2_SetFont(&s_u8g2, u8g2_font_5x7_tr);
    u8g2_DrawStr(&s_u8g2, (u8g2_uint_t)((128U - u8g2_GetStrWidth(&s_u8g2, status)) / 2U),
                 63, status);

    u8g2_SendBuffer(&s_u8g2);
}

static void Task3_SensorPage(void)
{
    uint32_t start = HAL_GetTick();
    uint32_t last_read = start - 1000U;
    uint32_t last_log = start;
    float t = 0.0f;
    float h = 0.0f;
    uint8_t valid = 0;

    while ((uint32_t)(HAL_GetTick() - start) < PAGE_SENSOR_MS)
    {
        uint32_t now = HAL_GetTick();

        if ((uint32_t)(now - last_read) >= 500U)
        {
            float nt = 0.0f;
            float nh = 0.0f;
            last_read = now;

            if (AHT20_Read(&nt, &nh) == 0U)
            {
                t = nt;
                h = nh;
                valid = 1;
            }
            else
            {
                valid = 0;
            }

            Task3_DrawSensorPage(t, h, valid);

            if ((uint32_t)(now - last_log) >= 2000U)
            {
                int32_t ti = (int32_t)(t * 10.0f);
                int32_t hi = (int32_t)(h * 10.0f);
                last_log = now;
                Board_Printf("[Task3] T=%d.%d C  H=%d.%d %%\r\n",
                             (int)(ti / 10), (int)(ti % 10),
                             (int)(hi / 10), (int)(hi % 10));
            }
        }

        Board_Service();
        if (Board_KeyPressed())
        {
            return;
        }
        HAL_Delay(20);
    }
}

/* ---------------- 任务入口 ---------------- */

void Task3_Run(void)
{
    uint8_t page = 0;

    Board_Print("\r\n[Task3] U8g2 port on STM32F103 + SSD1306 + AHT20\r\n");

    if (HAL_I2C_IsDeviceReady(&hi2c1, OLED_I2C_ADDR, 3, 100) != HAL_OK)
    {
        Board_Print("[Task3] OLED not found at 0x78, check PB6/PB7 wiring.\r\n");
        while (1)
        {
            LED_TOGGLE();
            HAL_Delay(300);
        }
    }

    U8g2_Init();
    Board_Print("[Task3] U8g2 init done.\r\n");

    if (AHT20_Init() == 0U)
    {
        Board_Print("[Task3] AHT20 init OK.\r\n");
    }
    else
    {
        Board_Print("[Task3] AHT20 init failed, check 0x38.\r\n");
    }

    /* 开机画面 */
    {
        const char *logo = "U8g2";
        u8g2_uint_t w;
        u8g2_ClearBuffer(&s_u8g2);
        u8g2_SetFont(&s_u8g2, u8g2_font_ncenB14_tr);
        w = u8g2_GetStrWidth(&s_u8g2, logo);
        u8g2_DrawStr(&s_u8g2, (u8g2_uint_t)((128U - w) / 2U), 30, logo);
        u8g2_SetFont(&s_u8g2, u8g2_font_6x12_tr);
        u8g2_DrawStr(&s_u8g2, 34, 46, "STM32F103");
        u8g2_DrawFrame(&s_u8g2, 0, 0, 128, 64);
        u8g2_SendBuffer(&s_u8g2);
    }
    (void)Task3_Wait(1500);

    while (1)
    {
        switch (page)
        {
            case 0:
                Task3_DemoPage();
                break;
            case 1:
                Task3_InfoPage();
                break;
            case 2:
                Task3_ScrollPage();
                break;
            case 3:
                Task3_AvatarPage();
                break;
            default:
                Task3_SensorPage();
                break;
        }

        page = (uint8_t)((page + 1U) % 5U);
    }
}
