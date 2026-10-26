# STM32F103C8T6 OLED / AHT20 综合实验

> 任务一：自定义 SSD1306 驱动 + 16×16 汉字 + AHT20 + 滚动  
> 任务三：CubeMX HAL + 裁剪 U8g2 移植 + demo + 四向滚动 + 动画头像

| 项目 | 内容 |
|---|---|
| 姓名 | 徐正心 |
| 学号 | 632407030612 |
| 班级 | 通信 2403（EIE 2403） |
| 实验室 | 重庆交通大学物联网 205 实训室 |
| 主控 | STM32F103C8T6 最小系统板（Blue Pill，64 KB Flash / 20 KB RAM） |
| 显示 | 0.96 英寸 SSD1306 OLED，128×64，I2C 地址 0x3C（写地址 0x78） |
| 传感器 | AHT20 温湿度传感器，I2C 地址 0x38（写地址 0x70） |
| 开发环境 | Keil MDK 5 + ARMCC 5.06 + STM32F1xx HAL |

---

## 1. 功能一览

### 任务一：自定义驱动（工程 `Task1_SSD1306`）

- 从零编写 SSD1306 的 I2C 驱动：初始化序列、1 KB 显存、像素/直线/矩形/圆、5×7 ASCII、16×16 汉字、位图绘制。
- 汉字点阵由 Windows 宋体 16 px 生成，共 69 个 16×16 字形；ASCII 使用 Adafruit GFX 5×7 字库。
- 显示学生信息：姓名、学号、班级、实验室。
- 读取 AHT20 温湿度，每 500 ms 刷新一次，带 CRC8 校验和 3 次重试。
- 长文本 **软件滚动**：`Hello，欢迎来到重庆交通大学物联网205实训室！` 从右向左循环。
- SSD1306 **硬件滚动**：先水平左滚，再切换为“左 + 垂直”对角线滚动，页 0/7 固定。

### 任务三：U8g2 移植（工程 `Task3_U8g2`）

- 使用 CubeMX 风格 HAL 工程（`OLED_AHT20.ioc` 为参考配置）。
- 将 U8g2 裁剪为 30 个源文件，只保留 SSD1306 128×64 I2C 全缓冲驱动和 6 种字体。
- 通过 `U8x8` 字节回调把 U8g2 接到 STM32 HAL 的 `HAL_I2C_Master_Transmit()`。
- U8g2 demo：标题、矩形、圆、指针、进度条、多种内置字体。
- 学号姓名页：U8g2 字体 + 自绘 16×16 汉字混合排版。
- 四向滚动：左、右、上、下四个方向循环，每 3 秒切换。
- 动画头像：56×56 两帧头像，2 秒周期，睁眼 1.55 秒 + 眨眼 0.45 秒。
- AHT20 温湿度页复用同一传感器驱动。

---

## 2. 硬件接线

| 模块/器件 | 引脚 | STM32F103C8T6 | 说明 |
|---|---|---|---|
| SSD1306 OLED | VCC | 3.3 V | 不要接 5 V |
| SSD1306 OLED | GND | GND | 共地 |
| SSD1306 OLED | SCL | PB6 | I2C1_SCL，400 kHz |
| SSD1306 OLED | SDA | PB7 | I2C1_SDA，400 kHz |
| AHT20 | VCC | 3.3 V | 与 OLED 并联在同一条 I2C 总线 |
| AHT20 | GND | GND | 共地 |
| AHT20 | SCL | PB6 | 与 OLED 共用 |
| AHT20 | SDA | PB7 | 与 OLED 共用 |
| USB-TTL（可选） | RX | PA9 | USART1_TX，115200-8-N-1 |
| USB-TTL（可选） | TX | PA10 | USART1_RX |
| 板载 LED | — | PC13 | 低电平点亮，500 ms 心跳 |
| 按键 | 一端 | PA0 | 内部上拉，按下接地 |
| 按键 | 另一端 | GND | — |

> I2C 总线需要上拉电阻。常见 OLED/AHT20 模块自带上拉；若通信不稳定，可在 SDA/SCL 上各加 4.7 kΩ 上拉到 3.3 V。

---

## 3. 目录结构

```
STM32F10x_OLED_AHT20_U8g2/
├─ Core/
│  ├─ Inc/                    应用与驱动头文件
│  │  ├─ app_config.h         地址、学生信息、页面时长
│  │  ├─ oled_ssd1306.h       自定义 OLED 驱动接口
│  │  ├─ font_cn.h            16×16 汉字字库接口
│  │  ├─ aht20.h              AHT20 驱动接口
│  │  ├─ avatar.h             头像与图标点阵
│  │  └─ task1_app.h / task3_app.h
│  └─ Src/
│     ├─ main.c               72 MHz 时钟、I2C1、USART1、GPIO
│     ├─ task1_app.c          任务一页面与滚动逻辑
│     ├─ task3_app.c          任务三 U8g2 移植与页面
│     ├─ oled_ssd1306.c       SSD1306 显存与绘图实现
│     ├─ font_cn.c            69 个 16×16 汉字点阵
│     ├─ font5x7.c            95 个 5×7 ASCII 字形
│     ├─ avatar.c             两帧头像、温度计、水滴图标
│     ├─ aht20.c              AHT20 初始化、测量、CRC8
│     └─ board.c              LED 心跳、按键消抖、串口打印
├─ Drivers/                   CMSIS、STM32F1xx HAL、裁剪版 U8g2
│  └─ U8g2/                   30 个源文件 + 裁剪说明
├─ MDK-ARM/
│  ├─ Task1_SSD1306.uvprojx   任务一工程
│  ├─ Task3_U8g2.uvprojx      任务三工程
│  ├─ Objects_Task1/          任务一构建输出（含 HEX）
│  └─ Objects_Task3/          任务三构建输出（含 HEX）
├─ Firmware/                  已编译的 HEX（方便直接烧录）
├─ docs/
│  ├─ 实验报告.md             实验报告源文件
│  ├─ 实验报告.pdf            实验报告 PDF
│  ├─ blog_oled_aht20.md      博客文章
│  └─ images/                 温湿度效果图、I2C 波形示意图
├─ tools/                     字库/头像/效果图生成脚本
├─ OLED_AHT20.ioc             CubeMX 参考配置
└─ README.md
```

---

## 4. 编译与烧录

### 4.1 编译

1. 用 Keil uVision 5 打开 `MDK-ARM/Task1_SSD1306.uvprojx` 或 `MDK-ARM/Task3_U8g2.uvprojx`。
2. 确认器件为 `STM32F103C8`，编译器为 ARMCC 5。
3. `Project → Build Target`（F7）。
4. 生成的 HEX 位于：
   - `MDK-ARM/Objects_Task1/Task1_SSD1306.hex`
   - `MDK-ARM/Objects_Task3/Task3_U8g2.hex`

两个工程共用 `Core/`、`Drivers/` 源码，通过预处理宏区分应用：

| 工程 | 预处理宏 | 应用入口 |
|---|---|---|
| `Task1_SSD1306` | `USE_HAL_DRIVER, STM32F103xB` | `Task1_Run()` |
| `Task3_U8g2` | `USE_HAL_DRIVER, STM32F103xB, TASK3_BUILD` | `Task3_Run()` |

> 注意：不要在一个工程里同时编译 `task1_app.c` 和 `task3_app.c`，否则会重复定义 `main` 之外的页面函数并浪费空间。本项目采用“两个单目标工程”的方式隔离。

### 4.2 烧录

任选一种方式：

- **ST-Link**：Keil 中 `Options for Target → Debug` 选择 ST-Link，`Flash → Download`（F8）。
- **串口 ISP**：用 FlyMcu / STM32CubeProgrammer，BOOT0 接 3.3 V，按复位后下载，完成后 BOOT0 接 GND 再复位。
- **直接使用 HEX**：`Firmware/Task1_SSD1306.hex` 或 `Firmware/Task3_U8g2.hex`。

### 4.3 运行

- 上电后先显示 1.5 秒开机画面，然后自动轮播页面。
- 按 PA0 可立即跳到下一页。
- USART1 每 115200 输出启动信息；任务三的温湿度页每 2 秒打印一次测量值。
- PC13 LED 以 500 ms 周期闪烁，表示主循环正常。

页面顺序与时长：

| 工程 | 页面 | 时长 |
|---|---|---|
| 任务一 | 学生信息 | 6 s |
| 任务一 | AHT20 温湿度 | 6 s |
| 任务一 | 长文本软件滚动 | 12 s |
| 任务一 | SSD1306 硬件滚动 | 8 s |
| 任务三 | U8g2 demo | 5 s |
| 任务三 | 学号姓名 | 6 s |
| 任务三 | 四向滚动 | 12 s |
| 任务三 | 动画头像 | 6 s |
| 任务三 | AHT20 温湿度 | 6 s |

---

## 5. 关键实现

### 5.1 自定义 SSD1306 驱动

- 显存按页组织：`s_gram[8][128]`，共 1024 字节；`OLED_Display()` 用一次“设置页地址 + 连续写 128 字节”的方式刷新。
- 初始化序列包含 `0xAE` 关显示、`0xD5` 时钟分频、`0xA8 0x3F` 复用率、`0x8D 0x14` 电荷泵、`0x20 0x00` 水平寻址、`0xA1/0xC8` 方向、`0xAF` 开显示。
- `OLED_DrawPixel()` 按 `y >> 3` 定位页、`1 << (y & 7)` 定位位，所有图元都建立在它之上。
- 汉字以 16×16 行优先点阵存储；ASCII 在 16 px 行内垂直居中，和汉字混排时基线一致。

### 5.2 字库生成

- `tools/gen_font_cn.ps1` 用 GDI+ 以宋体 16 px 渲染 `tools/chars_cn.txt` 中的汉字，输出 `font_cn.c` 的 `uint8_t[32]` 点阵。
- `tools/gen_font5x7.ps1` 生成 Adafruit GFX 兼容的 5×7 字库。
- 界面文字用 `uint16_t` Unicode 码点数组表示，避免 Keil 对 UTF-8 中文字面量编码不一致的问题。

### 5.3 软件滚动与硬件滚动

- **软件滚动**：每 25 ms 重绘一帧，偏移量按 55 px/s 增长；文本宽度 336 px，屏幕宽 128 px，总行程 464 px，循环显示。
- **硬件滚动**：直接写 SSD1306 的滚动命令。
  - `0x26/0x27`：水平右/左滚动；
  - `0x29/0x2A`：垂直 + 右/左滚动；
  - `0x2F` 启动，`0x2E` 停止；
  - 滚动区域设为页 1~6，页 0 和页 7 保持固定，因此标题和底部说明不会移动。
- 硬件滚动只能滚动显存中的静态内容，不能直接滚动任意长文本，所以长文本页使用软件滚动。

### 5.4 AHT20 驱动

- 上电等待 40 ms，读状态寄存器；若 bit3（校准位）为 0，发送 `0xBE 0x08 0x00` 初始化。
- 测量命令 `0xAC 0x33 0x00`，等待 80 ms，连续读 7 字节。
- 湿度：`raw_h = (b1<<16 | b2<<8 | b3) >> 4`，`H = raw_h × 100 / 2^20`。
- 温度：`raw_t = ((b3 & 0x0F)<<16 | b4<<8 | b5)`，`T = raw_t × 200 / 2^20 − 50`。
- CRC8 多项式 `0x31`，初值 `0xFF`；校验失败只累计错误计数，不丢弃本次数据，避免个别模块 CRC 实现差异导致一直显示错误。
- 失败重试 3 次，仍失败则显示 `AHT20 ERR`。

### 5.5 U8g2 移植

- 使用全缓冲接口 `u8g2_Setup_ssd1306_i2c_128x64_noname_f()`，缓冲区 1 KB。
- 不启用 U8g2 自带的软件 I2C，而是在 `U8g2_ByteCb()` 中处理 `START / SEND / END` 消息，把数据拼到 160 字节缓冲后调用 `HAL_I2C_Master_Transmit()`。
- 删除无用驱动、字体和 `mui/u8log` 等模块，只保留 30 个源文件；详见 `Drivers/U8g2/README.md`。
- 自绘汉字通过 `u8g2_DrawXBMP()` 绘制；四向滚动时用 `u8g2_SetClipWindow()` 限制视口，防止文字覆盖标题和底部方向标签。

---

## 6. 效果图与说明

### 6.1 AHT20 温湿度页

![AHT20 温湿度页（程序渲染示意图）](docs/images/task1_sensor.png)

> 上图由 `tools/gen_report_images.ps1` 按程序实际字库和坐标渲染，用于展示页面布局，**不是硬件实拍**。实际 OLED 为单色发光，对比度和亮度以实物为准。

### 6.2 I2C 波形示意图

![I2C 地址 0x78 + 数据 0xA5 时序示意图](docs/images/i2c_waveform.png)

> 上图为按 I2C 规范绘制的示意图，用于回答“Keil 虚拟逻辑分析仪中 SDA 会显示什么”。图中包含 START、地址 0x78、ACK、数据 0xA5、ACK、STOP。

---

## 7. 编译结果

| 工程 | Code | RO-data | RW-data | ZI-data | Flash 估算 | RAM 估算 | 结果 |
|---|---:|---:|---:|---:|---:|---:|---|
| `Task1_SSD1306` | 14168 | 3748 | 28 | 2340 | 17.5 KB | 2.3 KB | 0 error / 0 warning |
| `Task3_U8g2` | 19200 | 9828 | 32 | 2896 | 28.4 KB | 2.9 KB | 0 error / 0 warning |

- Flash 估算 = Code + RO-data + RW-data；RAM 估算 = RW-data + ZI-data。
- STM32F103C8T6 提供 64 KB Flash / 20 KB RAM，两个工程均有充足余量。

---

## 8. 文档

- 实验报告（Markdown）：[`docs/实验报告.md`](docs/实验报告.md)
- 实验报告（PDF）：[`docs/实验报告.pdf`](docs/实验报告.pdf)
- 博客文章：[`docs/blog_oled_aht20.md`](docs/blog_oled_aht20.md)

---

## 9. 第三方与许可

- **STM32F1xx HAL / CMSIS**：STMicroelectronics，BSD 3-Clause。
- **U8g2**：Oliver Kraus，BSD 2-Clause；本仓库仅做文件裁剪，未修改核心算法，原版权声明保留在源文件中。
- **Adafruit GFX 5×7 字库**：Adafruit Industries，BSD 许可。
- 本项目应用层代码、字库生成脚本和文档由徐正心编写，采用 MIT License，供课程学习使用。

详细许可见 [`LICENSE`](LICENSE) 和 [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)。
