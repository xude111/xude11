# 第三方组件与许可说明

本项目在课程实验中使用并分发了以下第三方组件。各组件版权归原作者所有，使用时请遵守对应许可。

## 1. STM32F1xx HAL 驱动与 CMSIS

- 位置：`Drivers/STM32F1xx_HAL_Driver/`、`Drivers/CMSIS/`
- 版权：STMicroelectronics
- 许可：BSD 3-Clause License
- 说明：ST 官方 HAL 库和 CMSIS 设备头文件，未修改核心实现。

## 2. U8g2

- 位置：`Drivers/U8g2/`
- 版权：Copyright (c) Oliver Kraus
- 许可：BSD 2-Clause License
- 说明：本仓库中的 U8g2 为裁剪版，只保留 SSD1306 128×64 I2C 全缓冲驱动、6 种字体和 demo 所需模块；未修改核心算法，原版权声明保留在源文件中。
- 项目主页：https://github.com/olikraus/u8g2

## 3. Adafruit GFX 5×7 字库

- 位置：`Core/Src/font5x7.c`
- 版权：Copyright (c) Adafruit Industries
- 许可：BSD License
- 说明：5×7 ASCII 点阵字库，按 Adafruit GFX 的 `glcdfont` 格式使用。
- 项目主页：https://github.com/adafruit/Adafruit-GFX-Library

## 4. 汉字点阵

- 位置：`Core/Src/font_cn.c`
- 说明：由 `tools/gen_font_cn.ps1` 使用 Windows 宋体 16 px 渲染生成，仅收录本实验界面用到的 69 个汉字，供课程学习使用。
- 注意：Windows 字体本身受微软字体许可约束；如需商业使用，请自行确认字体授权或改用开源字体重新生成。

## 5. 本项目应用代码

- 位置：`Core/` 中除第三方字库外的应用与驱动代码、`README.md`、`docs/`、`tools/`
- 作者：徐正昕
- 许可：MIT License（见仓库根目录 `LICENSE`）
- 说明：可自由用于学习和二次开发，请保留版权声明。

---

如发现遗漏的第三方组件，请通过 GitHub Issue 联系补充。
