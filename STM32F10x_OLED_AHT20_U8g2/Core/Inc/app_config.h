/*
 * app_config.h - 工程公共配置
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

/* I2C 从机地址（HAL 使用 8 位写地址） */
#define OLED_I2C_ADDR       0x78U   /* SSD1306 7 位地址 0x3C */
#define AHT20_I2C_ADDR      0x70U   /* AHT20   7 位地址 0x38 */

/* 学生信息 */
#define STUDENT_ID_STR      "632407030612"
#define STUDENT_CLASS_STR   "EIE 2403"
#define LAB_NAME_STR        "CQJTU IoT 205"

/* 界面停留时间（毫秒） */
#define PAGE_INFO_MS        6000U
#define PAGE_SENSOR_MS      6000U
#define PAGE_AVATAR_MS      6000U
#define PAGE_DEMO_MS        5000U

/* 长文本滚动参数 */
#define SCROLL_STEP_PX      2
#define SCROLL_DELAY_MS     25U

/* 调试串口开关：1=输出调试信息，0=关闭 */
#define APP_DEBUG_UART      1

#endif /* APP_CONFIG_H */
