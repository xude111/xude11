# STM32F103 + uC/OS-II + AHT20 多任务采集

重庆交通大学 信息科学与工程学院 通信 2403 班  
徐正昕　632407030612

## 一、项目内容

本项目把 uC/OS-II 移植到 STM32F103C8T6，并完成三个任务：

| 任务 | 优先级 | 周期 | 功能 |
|---|---|---|---|
| Task_LED1 | 5 | 1 s | PA0 红灯亮 0.5 s、灭 0.5 s |
| Task_LED2 | 6 | 3 s | PB0 绿灯亮 1.5 s、灭 1.5 s |
| Task_AHT20 | 7 | 2 s | 通过 I2C1 读取 AHT20 温湿度，并从 USART1 发送 |

串口格式：

```text
hello uc/OS! AHT20: T=25.34 C, H=60.12 %RH
```

串口参数：115200-8-N-1。

## 二、硬件连接

| 信号 | STM32 引脚 | 说明 |
|---|---|---|
| 红灯 LED1 | PA0 | 220 Ω 到 LED 到 GND，高电平点亮 |
| 绿灯 LED2 | PB0 | 220 Ω 到 LED 到 GND，高电平点亮 |
| AHT20 SCL | PB6 | I2C1_SCL，400 kHz |
| AHT20 SDA | PB7 | I2C1_SDA |
| 串口 TX | PA9 | 接 USB-TTL RXD |
| 串口 RX | PA10 | 接 USB-TTL TXD |
| 电源 | 3.3 V / GND | AHT20 与 MCU 共地 |

AHT20 模块通常自带 4.7 kΩ 上拉；若使用裸传感器，需要在 SCL/SDA 外接上拉电阻。

## 三、目录结构

```text
STM32F10x_uCOSII_RTOS/
├─ App/
│  ├─ app.c / app.h         三个应用任务
│  ├─ aht20.c / aht20.h     AHT20 驱动（OSTimeDlyHMSM 版本）
├─ Core/
│  ├─ Inc/                  main.h、HAL 配置
│  └─ Src/                  main.c、stm32f1xx_it.c、msp
├─ Drivers/                 STM32F1 HAL 与 CMSIS
├─ Middlewares/uCOSII/
│  ├─ Source/               uC/OS-II V2.93.01 内核
│  ├─ Ports/                Cortex-M3/ARMCC 移植层
│  └─ Cfg/                  os_cfg.h、app_cfg.h、includes.h
├─ MDK-ARM/
│  ├─ uCOSII_RTOS.uvprojx   Keil 工程
│  └─ Objects/              编译输出、HEX
└─ docs/                    实验报告、博客、图片
```

## 四、编译方法

1. 用 Keil MDK 打开 `MDK-ARM/uCOSII_RTOS.uvprojx`；
2. 编译器选择 ARMCC V5.06（AC5）；
3. 全量重建，输出 `MDK-ARM/Objects/uCOSII_RTOS.hex`。

本机编译结果：

```text
Code=13928  RO-data=568  RW-data=64  ZI-data=7224
0 Error(s), 0 Warning(s)
```

## 五、运行方法

1. ST-Link 连接 SWD，下载 `uCOSII_RTOS.hex`；
2. 按上表连接 LED、AHT20 和 USB-TTL；
3. 串口助手选择 115200-8-N-1；
4. 上电后应看到 PA0 以 1 s 周期闪烁、PB0 以 3 s 周期闪烁，串口每 2 s 输出一行温湿度。

> 说明：当前开发机未识别到 ST-Link 和 USB-TTL 设备，仓库中的结果是编译/链接验证；实机现象需在实验台烧录后确认。

## 六、移植要点

- uC/OS-II 使用 `os_cpu.h`、`os_cpu_a.asm`、`os_cpu_c.c` 三个 Cortex-M3 端口文件；
- `SysTick_Handler` 同时调用 `HAL_IncTick()` 和 `OS_CPU_SysTickHandler()`；
- `PendSV_Handler` 调用 `OS_CPU_PendSVHandler()`；
- `OS_TASK_SW_HOOK_EN=1`，否则汇编中的 `OSTaskSwHook` 链接失败；
- STM32F103 无 FPU，`OS_CPU_ARM_FP_EN=0`；
- AHT20 使用硬件 I2C1，地址 0x38（HAL 8 位写地址 0x70）；
- AHT20 的 40/10/80 ms 等待改为 `OSTimeDlyHMSM`，避免阻塞其他任务。

## 七、文档

- 实验报告：`docs/实验报告_STM32F103_uCOSII_AHT20.pdf`
- Word 版：`docs/实验报告_STM32F103_uCOSII_AHT20.docx`
- 博客源文件：`docs/blog_uCOSII_AHT20.md`

GitHub：<https://github.com/xude111/xude11/tree/main/STM32F10x_uCOSII_AHT20>
