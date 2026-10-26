/*
 * task1_app.c - 任务 1：自定义 SSD1306 驱动 + 16x16 汉字 + AHT20 + 滚动
 *
 * 页面流程：
 *   1. 学生信息页（姓名、学号、班级、实验室）
 *   2. AHT20 温湿度页
 *   3. 长文本软件水平滚动页
 *   4. SSD1306 硬件滚动演示页（水平 + 对角线）
 *
 * 按键 PA0 可随时切换到下一页；PC13 LED 500ms 心跳。
 */
#include "main.h"
#include "task1_app.h"
#include "app_config.h"
#include "board.h"
#include "aht20.h"
#include "oled_ssd1306.h"
#include "font5x7.h"
#include "font_cn.h"
#include "avatar.h"
#include <stdio.h>

/* ---------------- 界面文字（Unicode 码点，避免源文件编码问题） ---------------- */

static const uint16_t TXT_NAME[] =
{
    0x5F90, 0x6B63, 0x5FC3, 0   /* 徐正心 */
};

static const uint16_t TXT_CLASS[] =
{
    0x901A, 0x4FE1, '2', '4', '0', '3', 0   /* 通信2403 */
};

static const uint16_t TXT_INFO_TITLE[] =
{
    0x5B66, 0x751F, 0x4FE1, 0x606F, 0   /* 学生信息 */
};

static const uint16_t TXT_SENSOR_TITLE[] =
{
    0x6E29, 0x6E7F, 0x5EA6, 0x68C0, 0x6D4B, 0   /* 温湿度检测 */
};

static const uint16_t TXT_LONG_TITLE[] =
{
    0x957F, 0x6587, 0x672C, 0x6EDA, 0x52A8, 0   /* 长文本滚动 */
};

/* Hello，欢迎来到重庆交通大学物联网205实训室！ */
static const uint16_t TXT_LONG_SCROLL[] =
{
    'H', 'e', 'l', 'l', 'o',
    0xFF0C,
    0x6B22, 0x8FCE, 0x6765, 0x5230,
    0x91CD, 0x5E86, 0x4EA4, 0x901A, 0x5927, 0x5B66,
    0x7269, 0x8054, 0x7F51,
    '2', '0', '5',
    0x5B9E, 0x8BAD, 0x5BA4,
    0xFF01,
    0
};

/* ---------------- 工具函数 ---------------- */

/**
  * @brief 等待指定时间，期间处理按键与 LED 心跳；按键返回 1
  */
static uint8_t Task1_Wait(uint32_t ms)
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

/**
  * @brief 温度格式化：-12.3 C / 25.6 C
  */
static void Task1_FormatTemp(char *buf, float t)
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

/**
  * @brief 湿度格式化：55.2 %
  */
static void Task1_FormatHumi(char *buf, float h)
{
    int32_t v = (int32_t)(h * 10.0f);
    if (v < 0)
    {
        v = 0;
    }
    sprintf(buf, "%d.%d %%", (int)(v / 10), (int)(v % 10));
}

/* ---------------- 页面 1：学生信息 ---------------- */

static void Task1_DrawInfoPage(void)
{
    OLED_Clear();

    /* 顶部标题：学生信息 */
    OLED_DrawTextU16(32, 0, TXT_INFO_TITLE, OLED_TEXT_MIXED16, 1);
    OLED_DrawHLine(0, 17, 128, 1);

    /* 姓名（16x16 汉字） */
    OLED_DrawTextU16(40, 21, TXT_NAME, OLED_TEXT_MIXED16, 1);

    /* 学号 */
    OLED_DrawString5x7(28, 39, STUDENT_ID_STR, 1);

    /* 班级 */
    OLED_DrawTextU16(36, 47, TXT_CLASS, OLED_TEXT_MIXED16, 1);

    OLED_Display();
}

static void Task1_InfoPage(void)
{
    Task1_DrawInfoPage();
    (void)Task1_Wait(PAGE_INFO_MS);
}

/* ---------------- 页面 2：AHT20 温湿度 ---------------- */

static void Task1_DrawSensorPage(float t, float h, uint8_t valid)
{
    char tbuf[16];
    char hbuf[16];
    char status[20];

    if (valid)
    {
        Task1_FormatTemp(tbuf, t);
        Task1_FormatHumi(hbuf, h);
        sprintf(status, "AHT20 OK");
    }
    else
    {
        sprintf(tbuf, "--.- C");
        sprintf(hbuf, "--.- %%");
        sprintf(status, "AHT20 ERR");
    }

    OLED_Clear();

    OLED_DrawTextU16(24, 0, TXT_SENSOR_TITLE, OLED_TEXT_MIXED16, 1);
    OLED_DrawHLine(0, 17, 128, 1);

    /* 温度 */
    OLED_DrawBitmap(6, 22, 16, 16, icon_temp);
    OLED_DrawString5x7Scaled(28, 23, tbuf, 2, 1);

    /* 湿度 */
    OLED_DrawBitmap(6, 40, 16, 16, icon_humi);
    OLED_DrawString5x7Scaled(28, 41, hbuf, 2, 1);

    /* 状态 */
    OLED_DrawString5x7(40, 57, status, 1);

    OLED_Display();
}

static void Task1_SensorPage(void)
{
    uint32_t start = HAL_GetTick();
    uint32_t last_read = start - 1000U;
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

            Task1_DrawSensorPage(t, h, valid);
        }

        Board_Service();
        if (Board_KeyPressed())
        {
            return;
        }
        HAL_Delay(20);
    }
}

/* ---------------- 页面 3：长文本软件滚动 ---------------- */

static void Task1_ScrollPage(void)
{
    uint32_t start = HAL_GetTick();
    uint16_t text_w = OLED_TextWidthU16(TXT_LONG_SCROLL);
    uint32_t travel = (uint32_t)text_w + OLED_WIDTH;

    while ((uint32_t)(HAL_GetTick() - start) < 12000U)
    {
        uint32_t elapsed = (uint32_t)(HAL_GetTick() - start);
        uint32_t pos = (elapsed * 55U) / 1000U;   /* 55 像素/秒 */
        int16_t x;

        if (travel > 0U)
        {
            pos %= travel;
        }
        x = (int16_t)((int32_t)OLED_WIDTH - (int32_t)pos);

        OLED_Clear();
        OLED_DrawTextU16(24, 0, TXT_LONG_TITLE, OLED_TEXT_MIXED16, 1);
        OLED_DrawHLine(0, 17, 128, 1);

        /* 长文本从右向左滚动；超出屏幕的部分由驱动自动裁剪 */
        OLED_DrawTextU16(x, 24, TXT_LONG_SCROLL, OLED_TEXT_MIXED16, 1);

        OLED_DrawHLine(0, 44, 128, 1);
        OLED_DrawString5x7(16, 57, "SOFTWARE SCROLL", 1);
        OLED_Display();

        Board_Service();
        if (Board_KeyPressed())
        {
            return;
        }
        HAL_Delay(25);
    }
}

/* ---------------- 页面 4：SSD1306 硬件滚动 ---------------- */

static void Task1_DrawHwContent(void)
{
    OLED_Clear();

    /* 页 0：固定标题，不参与硬件滚动 */
    OLED_DrawString5x7(19, 0, "HARDWARE SCROLL", 1);

    /* 页 1~6：参与硬件滚动的静态图案 */
    OLED_DrawString5x7(8, 16, "SSD1306 0123456789", 1);
    OLED_DrawString5x7(4, 26, "HARDWARE SCROLL DEMO", 1);
    OLED_DrawString5x7(4, 36, "<<< LEFT / RIGHT >>>", 1);
    OLED_DrawString5x7(0, 46, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 1);

    /* 页 7：固定底部说明 */
    OLED_DrawString5x7(10, 57, "128x64  I2C 400kHz", 1);

    OLED_Display();
}

static void Task1_HwScrollPage(void)
{
    uint32_t start = HAL_GetTick();
    uint8_t phase = 0;
    uint32_t phase_start = start;

    OLED_ScrollStop();
    Task1_DrawHwContent();
    OLED_HScrollStart(1, 1, 6, 0);   /* 向左水平滚动，interval=0 为最快 */

    while ((uint32_t)(HAL_GetTick() - start) < 8000U)
    {
        uint32_t now = HAL_GetTick();

        if (phase == 0U && (uint32_t)(now - phase_start) >= 4000U)
        {
            /* 切换到左 + 垂直对角线滚动 */
            OLED_ScrollStop();
            phase = 1;
            phase_start = now;
            Task1_DrawHwContent();
            OLED_VScrollStart(1, 1, 6, 0, 1);
        }

        Board_Service();
        if (Board_KeyPressed())
        {
            break;
        }
        HAL_Delay(20);
    }

    OLED_ScrollStop();
}

/* ---------------- 任务入口 ---------------- */

void Task1_Run(void)
{
    uint8_t page = 0;

    Board_Print("\r\n[Task1] custom SSD1306 driver + 16x16 Chinese + AHT20\r\n");

    OLED_ScrollStop();
    if (OLED_Init() != 0U)
    {
        Board_Print("[Task1] OLED not found at 0x78, check PB6/PB7 wiring.\r\n");
        while (1)
        {
            LED_TOGGLE();
            HAL_Delay(300);
        }
    }

    if (AHT20_Init() == 0U)
    {
        Board_Print("[Task1] AHT20 init OK.\r\n");
    }
    else
    {
        Board_Print("[Task1] AHT20 init failed, check 0x38.\r\n");
    }

    /* 开机画面 */
    OLED_Clear();
    OLED_DrawString5x7Scaled(31, 6, "STM32F103", 2, 1);
    OLED_DrawString5x7Scaled(37, 24, "SSD1306", 2, 1);
    OLED_DrawTextU16(40, 44, TXT_NAME, OLED_TEXT_MIXED16, 1);
    OLED_Display();
    (void)Task1_Wait(1500);

    while (1)
    {
        switch (page)
        {
            case 0:
                Task1_InfoPage();
                break;
            case 1:
                Task1_SensorPage();
                break;
            case 2:
                Task1_ScrollPage();
                break;
            default:
                Task1_HwScrollPage();
                break;
        }

        page = (uint8_t)((page + 1U) % 4U);
    }
}
