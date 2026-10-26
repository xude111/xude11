# 固件说明

本目录提供两个已编译的 HEX 文件，可直接烧录到 STM32F103C8T6：

| 文件 | 对应工程 | 功能 |
|---|---|---|
| `Task1_SSD1306.hex` | `MDK-ARM/Task1_SSD1306.uvprojx` | 自定义 SSD1306 驱动、学生信息、AHT20、软件滚动、硬件滚动 |
| `Task3_U8g2.hex` | `MDK-ARM/Task3_U8g2.uvprojx` | 裁剪 U8g2、demo、学号姓名、四向滚动、动画头像、AHT20 |

## 烧录方法

### 方法一：ST-Link + Keil

1. 打开对应 `.uvprojx` 工程。
2. `Options for Target → Debug` 选择 ST-Link Debugger。
3. `Flash → Download`（F8）。

### 方法二：STM32CubeProgrammer

1. 连接 ST-Link。
2. 选择 `Erasing & Programming`。
3. 加载对应 HEX 文件，起始地址 `0x08000000`。
4. 点击 `Start Programming`。

### 方法三：串口 ISP

1. BOOT0 接 3.3 V，BOOT1 接 GND。
2. 按复位键进入系统存储器启动模式。
3. 用 FlyMcu 或 STM32CubeProgrammer 通过 USART1（PA9/PA10）下载。
4. 下载完成后 BOOT0 接回 GND，再按复位键运行。

## 注意

- 供电必须为 3.3 V，OLED 和 AHT20 都不要接 5 V。
- I2C1 使用 PB6（SCL）/ PB7（SDA），总线上需要上拉电阻。
- 两个 HEX 功能不同，同一时间只能烧录一个；重新烧录会覆盖另一个。
- HEX 为文本格式，文件大小约 50 KB / 82 KB，实际二进制占用见实验报告的资源表。
