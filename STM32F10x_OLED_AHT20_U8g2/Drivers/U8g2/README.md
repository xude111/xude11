# U8g2 裁剪说明

本目录是 [U8g2](https://github.com/olikraus/u8g2) 的**最小裁剪版**，只保留本实验实际编译的源文件。原始完整库中的 108 个未使用文件已移除，以缩小编译体积、突出“裁剪移植”的过程。

## 保留的模块

| 类别 | 文件 |
|---|---|
| 公共头文件 | `u8g2.h`、`u8x8.h` |
| U8g2 核心 | `u8g2_setup.c`、`u8g2_buffer.c`、`u8g2_hvline.c`、`u8g2_ll_hvline.c`、`u8g2_intersection.c` |
| 图形函数 | `u8g2_box.c`、`u8g2_line.c`、`u8g2_circle.c`、`u8g2_arc.c`、`u8g2_polygon.c` |
| 位图/字体 | `u8g2_bitmap.c`、`u8g2_font.c`、`u8g2_kerning.c`、`u8g2_cleardisplay.c` |
| 显示设备 | `u8g2_d_setup_min.c`、`u8g2_d_memory_min.c`、`u8x8_d_ssd1306_128x64_noname.c` |
| 底层通信 | `u8x8_setup.c`、`u8x8_byte.c`、`u8x8_cad.c`、`u8x8_display.c`、`u8x8_gpio.c`、`u8x8_capture.c` |
| 字符串工具 | `u8x8_string.c`、`u8x8_u8toa.c`、`u8x8_u16toa.c`、`u8x8_8x8.c` |
| 字体数据 | `u8g2_fonts_min.c`（仅 5x7、6x10、6x12、7x14B、ncenB10、ncenB14） |

## 裁剪原则

1. 只保留 SSD1306 128×64 I2C 一种显示驱动，删除 SH1106、ST7565、UC1608 等其余驱动。
2. 只保留全缓冲（`_f`）接口 `u8g2_Setup_ssd1306_i2c_128x64_noname_f`，删除 1/2 页缓冲接口。
3. 只保留 demo 与界面用到的 6 种字体，删除其余数百种字体（原 `u8x8_fonts.c` 约 1.6 MB）。
4. 删除 `mui`、`u8log`、`u8g2_message`、`u8g2_selection_list` 等与本实验无关的模块。
5. 底层 I2C 不直接使用 U8g2 自带的软件 I2C，而是在 `Core/Src/task3_app.c` 中实现 `U8x8` 字节回调，把数据交给 STM32 HAL 的 `HAL_I2C_Master_Transmit()`。

## 许可

U8g2 版权归 Oliver Kraus 所有，采用 BSD 2-Clause License。本目录仅做文件裁剪，未修改核心算法；`u8g2.h` 文件头保留了原始版权声明。使用时请遵守原库许可。
