# STM32F103C8T6 流水灯实验

嵌入式系统实验。用**寄存器方式**操作 STM32F103C8T6 最小系统板（Blue Pill）的 GPIO，
让面包板上的红、绿、蓝三只 LED 轮流闪烁，间隔 1 秒；再把板载 PC13 的 LED 也加进流水灯。

## 实现要点

* GPIOA / GPIOB / GPIOC 三个端口各用一根引脚驱动一只 LED，轮流点亮
* 系统时钟：外部 8MHz 晶振（HSE）经 PLL 9 倍频得到 72MHz
* 延时用 Cortex-M3 内核的 SysTick（轮询方式），不占用中断
* 不使用标准外设库和 CMSIS，直接读写 RCC / GPIO / SysTick 寄存器
* 板载 PC13 是共阳接法，低电平点亮，程序里用 `active_low` 标志处理

## 硬件接线

| 板子引脚 | LED | 限流电阻 | 另一端 |
|---|---|---|---|
| PA0 | 红色 | 220Ω | GND |
| PB0 | 绿色 | 220Ω | GND |
| PC14 | 蓝色 | 1kΩ | GND |
| PC13 | 板载 LED | 不用接线 | — |

下载器用 ST-Link V2：`3.3V→3V3`、`GND→GND`、`SWDIO→PA13`、`SWCLK→PA14`。

> 蓝灯 VF 较高，用 1kΩ 时电流偏小、亮度偏暗，想亮一点可以换成 470Ω。
> PC13/PC14/PC15 由备份域供电，最大输出电流只有 3mA，限流电阻不要太小。

## 编译与下载

1. 双击 `MDK-ARM/LED_Waterfall.uvprojx` 打开 Keil 工程
2. `F7` 编译，应为 0 Error 0 Warning
3. 接好 ST-Link，按 `F8` 下载
4. 按板子上的复位键，LED 开始轮流闪烁

没有 ST-Link 也可以用串口下载：USB-TTL 的 TXD→PA10、RXD→PA9、GND 共地，
下载时 BOOT0 拨到 1，用 FlyMcu 烧 `MDK-ARM/Objects/LED_Waterfall.hex`，烧完拨回 0。

## 文件说明

| 文件 | 说明 |
|---|---|
| `User/main.c` | 主程序，寄存器方式实现，有注释 |
| `User/stm32f103_regs.h` | 用到的寄存器地址和位定义 |
| `Start/startup_stm32f103_md.s` | 启动文件 |
| `MDK-ARM/LED_Waterfall.uvprojx` | Keil 工程 |
| `docs/实验报告.md` / `.pdf` | 实验报告 |

## 两个版本

改 `User/main.c` 开头的宏，可以切换三灯版和四灯版：

```c
#define EXP_VERSION   2      /* 1 = 三只 LED；2 = 再加上板载 PC13，共四只 */
```

## 运行结果

```
红(PA0) 亮 1 秒 → 绿(PB0) 亮 1 秒 → 蓝(PC14) 亮 1 秒 → 板载(PC13) 亮 1 秒 → 循环
```

编译结果：`Program Size: Code=856 RO-data=416 RW-data=52 ZI-data=1028`，占用 Flash 约 1.3 KB。
