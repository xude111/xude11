/**
  ******************************************************************************
  * @file    stm32f103_regs.h
  * @brief   STM32F103C8T6 寄存器方式编程所需的寄存器地址与位定义(自写)
  * @note    1. 本文件只包含"寄存器方式"编程所必需的最少内容, 每个寄存器都写明
  *             绝对地址, 方便对照《STM32F10xxx 参考手册 RM0008》逐条核对。
  *          2. 本工程不使用 ST 标准外设库(StdPeriph_Lib), 也不包含 CMSIS 的
  *             stm32f10x.h, 因此不会发生符号重复定义的冲突。
  *          3. 结构体成员顺序 = 寄存器在芯片内部的排列顺序, 因此只要结构体
  *             首地址正确, 就可以像 RM0008 一样用 "GPIOA->CRL" 的形式访问。
  ******************************************************************************
  */

#ifndef __STM32F103_REGS_H
#define __STM32F103_REGS_H

#include <stdint.h>

/* 与 CMSIS 保持一致的写法: 用 volatile 修饰, 防止编译器优化掉"看似无用"的读写 */
#ifndef __IO
#define __IO   volatile
#endif

/* ==========================================================================
 * 一、存储器映射 / 总线基地址
 *     (RM0008 第 2.3 节 存储器映射、第 3 章 存储器和总线架构)
 * ========================================================================== */
#define PERIPH_BASE        0x40000000UL   /* 外设区首地址                     */
#define APB1PERIPH_BASE    0x40000000UL   /* APB1 总线基地址(低速, 最高36MHz) */
#define APB2PERIPH_BASE    0x40010000UL   /* APB2 总线基地址(高速, 最高72MHz) */

/* ---------------------- 挂在 APB2 上的 GPIO ------------------------------ */
#define GPIOA_BASE         (APB2PERIPH_BASE + 0x00000800UL)   /* 0x40010800 */
#define GPIOB_BASE         (APB2PERIPH_BASE + 0x00000C00UL)   /* 0x40010C00 */
#define GPIOC_BASE         (APB2PERIPH_BASE + 0x00001000UL)   /* 0x40011000 */
#define GPIOD_BASE         (APB2PERIPH_BASE + 0x00001400UL)   /* 0x40011400 */
#define GPIOE_BASE         (APB2PERIPH_BASE + 0x00001800UL)   /* 0x40011800 */
#define AFIO_BASE          (APB2PERIPH_BASE + 0x00000000UL)   /* 0x40010000 */

/* ---------------------- 挂在 APB1 上的 RCC ------------------------------- */
#define RCC_BASE           (APB1PERIPH_BASE + 0x00021000UL)   /* 0x40021000 */

/* ---------------------- 其他常用地址 ------------------------------------- */
#define FLASH_R_BASE       0x40022000UL   /* FLASH 接口寄存器基地址(ACR 偏移 0x00) */
#define SYS_TICK_BASE      0xE000E010UL   /* Cortex-M3 内核 SysTick 寄存器基地址   */

/* ==========================================================================
 * 二、GPIO 寄存器结构体定义
 *     每个 GPIO 端口占用 0x400 字节, 内部 7 个寄存器依次为:
 *       CRL(0x00) CRH(0x04) IDR(0x08) ODR(0x0C) BSRR(0x10) BRR(0x14) LCKR(0x18)
 * ========================================================================== */
typedef struct
{
    __IO uint32_t CRL;    /*!< 0x00 端口配置低寄存器  : 配置 Px0 ~ Px7   */
    __IO uint32_t CRH;    /*!< 0x04 端口配置高寄存器  : 配置 Px8 ~ Px15  */
    __IO uint32_t IDR;    /*!< 0x08 端口输入数据寄存器: 只读, 读取引脚电平 */
    __IO uint32_t ODR;    /*!< 0x0C 端口输出数据寄存器: 写引脚输出电平     */
    __IO uint32_t BSRR;   /*!< 0x10 端口位置位/复位寄存器(原子操作)       */
    __IO uint32_t BRR;    /*!< 0x14 端口位复位寄存器(原子操作)            */
    __IO uint32_t LCKR;   /*!< 0x18 端口配置锁定寄存器                    */
} GPIO_TypeDef;

/* ==========================================================================
 * 三、RCC 寄存器结构体定义 (本实验只用到 CR / CFGR / APB2ENR, 其余一并列出)
 * ========================================================================== */
typedef struct
{
    __IO uint32_t CR;         /*!< 0x00 时钟控制寄存器             */
    __IO uint32_t CFGR;       /*!< 0x04 时钟配置寄存器             */
    __IO uint32_t CIR;        /*!< 0x08 时钟中断寄存器             */
    __IO uint32_t APB2RSTR;   /*!< 0x0C APB2 外设复位寄存器        */
    __IO uint32_t APB1RSTR;   /*!< 0x10 APB1 外设复位寄存器        */
    __IO uint32_t AHBENR;     /*!< 0x14 AHB  外设时钟使能寄存器    */
    __IO uint32_t APB2ENR;    /*!< 0x18 APB2 外设时钟使能寄存器    */
    __IO uint32_t APB1ENR;    /*!< 0x1C APB1 外设时钟使能寄存器    */
    __IO uint32_t BDCR;       /*!< 0x20 备份域控制寄存器           */
    __IO uint32_t CSR;        /*!< 0x24 控制/状态寄存器            */
} RCC_TypeDef;

/* ==========================================================================
 * 四、SysTick(内核嘀嗒定时器)寄存器结构体定义
 * ========================================================================== */
typedef struct
{
    __IO uint32_t CTRL;    /*!< 0x00 控制及状态寄存器 */
    __IO uint32_t LOAD;    /*!< 0x04 重装载数值寄存器 */
    __IO uint32_t VAL;     /*!< 0x08 当前数值寄存器   */
    __IO uint32_t CALIB;   /*!< 0x0C 校准数值寄存器   */
} SysTick_TypeDef;

/* ==========================================================================
 * 五、外设指针宏定义(把"地址"变成"可以点出来"的结构体指针)
 * ========================================================================== */
#define GPIOA              ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB              ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC              ((GPIO_TypeDef *) GPIOC_BASE)
#define RCC                ((RCC_TypeDef  *) RCC_BASE)
#define SysTick            ((SysTick_TypeDef *) SYS_TICK_BASE)

/* FLASH_ACR: 只有 1 个寄存器需要用到, 直接用指针读写, 不再建结构体 */
#define FLASH_ACR          (*(__IO uint32_t *)FLASH_R_BASE)

/* ==========================================================================
 * 六、位定义
 * ========================================================================== */

/* ---------------------------- 6.1 RCC_CR (0x40021000) -------------------- */
#define RCC_CR_HSION        (1UL << 0)     /* 内部 8MHz RC 振荡器使能       */
#define RCC_CR_HSIRDY       (1UL << 1)     /* 内部 8MHz RC 就绪标志         */
#define RCC_CR_HSEON        (1UL << 16)    /* 外部高速晶振使能              */
#define RCC_CR_HSERDY       (1UL << 17)    /* 外部高速晶振就绪标志          */
#define RCC_CR_HSEBYP       (1UL << 18)    /* 外部晶振旁路(接有源晶振时用)  */
#define RCC_CR_CSSON        (1UL << 19)    /* 时钟安全系统使能              */
#define RCC_CR_PLLON        (1UL << 24)    /* PLL 使能                      */
#define RCC_CR_PLLRDY       (1UL << 25)    /* PLL 就绪标志                  */

/* ---------------------------- 6.2 RCC_CFGR (0x40021004) ------------------ */
#define RCC_CFGR_SW              (0x3UL << 0)      /* 系统时钟源选择      */
#define RCC_CFGR_SW_HSI          (0x0UL << 0)      /*   00: HSI           */
#define RCC_CFGR_SW_HSE          (0x1UL << 0)      /*   01: HSE           */
#define RCC_CFGR_SW_PLL          (0x2UL << 0)      /*   10: PLL           */
#define RCC_CFGR_SWS             (0x3UL << 2)      /* 系统时钟源状态      */
#define RCC_CFGR_SWS_HSI         (0x0UL << 2)      /*   HSI 已作为系统时钟 */
#define RCC_CFGR_SWS_HSE         (0x1UL << 2)      /*   HSE 已作为系统时钟 */
#define RCC_CFGR_SWS_PLL         (0x2UL << 2)      /*   PLL 已作为系统时钟 */
#define RCC_CFGR_HPRE_DIV1       (0x0UL << 4)      /* AHB  不分频 -> 72MHz */
#define RCC_CFGR_PPRE1_DIV1      (0x0UL << 8)      /* APB1 不分频(会超频)  */
#define RCC_CFGR_PPRE1_DIV2      (0x4UL << 8)      /* APB1 2 分频 -> 36MHz */
#define RCC_CFGR_PPRE1_DIV4      (0x5UL << 8)      /* APB1 4 分频          */
#define RCC_CFGR_PPRE2_DIV1      (0x0UL << 11)     /* APB2 不分频 -> 72MHz */
#define RCC_CFGR_PPRE2_DIV2      (0x4UL << 11)     /* APB2 2 分频          */
#define RCC_CFGR_ADCPRE_DIV6     (0x2UL << 14)     /* ADC 预分频           */
#define RCC_CFGR_PLLSRC_HSE      (1UL << 16)       /* PLL 输入 = HSE 或 HSE/2 */
#define RCC_CFGR_PLLXTPRE_HSE_DIV2 (1UL << 17)     /* HSE 先 2 分频再进 PLL   */
#define RCC_CFGR_PLLMULL9        (0x7UL << 18)     /* PLL 9  倍频: 8M x 9 = 72MHz */
#define RCC_CFGR_USBPRE_DIV1_5   (1UL << 22)       /* USB 预分频 1.5        */

/* ---------------------------- 6.3 RCC_APB2ENR (0x40021018) --------------- */
#define RCC_APB2ENR_AFIOEN  (1UL << 0)     /* 复用功能 IO 时钟使能 */
#define RCC_APB2ENR_IOPAEN  (1UL << 2)     /* GPIOA 时钟使能       */
#define RCC_APB2ENR_IOPBEN  (1UL << 3)     /* GPIOB 时钟使能       */
#define RCC_APB2ENR_IOPCEN  (1UL << 4)     /* GPIOC 时钟使能       */
#define RCC_APB2ENR_IOPDEN  (1UL << 5)     /* GPIOD 时钟使能       */
#define RCC_APB2ENR_IOPEEN  (1UL << 6)     /* GPIOE 时钟使能       */

/* ---------------------------- 6.4 FLASH_ACR (0x40022000) ----------------- */
#define FLASH_ACR_LATENCY_0 (0x0UL << 0)   /* 0 个等待周期 (<=24MHz) */
#define FLASH_ACR_LATENCY_1 (0x1UL << 0)   /* 1 个等待周期 (<=48MHz) */
#define FLASH_ACR_LATENCY_2 (0x2UL << 0)   /* 2 个等待周期 (<=72MHz) */
#define FLASH_ACR_PRFTBE    (1UL << 4)     /* 预取缓冲区使能          */
#define FLASH_ACR_PRFTBS    (1UL << 5)     /* 预取缓冲区状态(只读)    */

/* ---------------------------- 6.5 SysTick_CTRL (0xE000E010) -------------- */
#define SYSTICK_CTRL_ENABLE     (1UL << 0)   /* 使能计数             */
#define SYSTICK_CTRL_TICKINT    (1UL << 1)   /* 计数到 0 时产生中断   */
#define SYSTICK_CTRL_CLKSOURCE  (1UL << 2)   /* 1: 时钟源 = HCLK      */
#define SYSTICK_CTRL_COUNTFLAG  (1UL << 16)  /* 计数到 0 的标志位     */

/* ==========================================================================
 * 七、GPIO 配置字说明(写入 CRL/CRH 的 4 位 = [CNF1 CNF0 MODE1 MODE0])
 *
 *   输出模式下:
 *      MODE[1:0] = 00 保留 / 01 输出 10MHz / 10 输出 2MHz / 11 输出 50MHz
 *      CNF[1:0]  = 00 通用推挽 / 01 通用开漏 / 10 复用推挽 / 11 复用开漏
 *   输入模式下:
 *      MODE[1:0] = 00 输入模式
 *      CNF[1:0]  = 00 模拟输入 / 01 浮空输入 / 10 上拉或下拉输入 / 11 保留
 * ========================================================================== */
#define GPIO_CNF_OUT_PP      0x0U   /* 通用推挽输出   */
#define GPIO_CNF_OUT_OD      0x1U   /* 通用开漏输出   */
#define GPIO_CNF_AF_PP       0x2U   /* 复用推挽输出   */
#define GPIO_CNF_AF_OD       0x3U   /* 复用开漏输出   */
#define GPIO_CNF_IN_ANALOG   0x0U   /* 模拟输入       */
#define GPIO_CNF_IN_FLOAT    0x1U   /* 浮空输入       */
#define GPIO_CNF_IN_PUPD     0x2U   /* 上拉/下拉输入  */

#define GPIO_MODE_IN         0x0U   /* 输入模式        */
#define GPIO_MODE_OUT_10MHZ  0x1U   /* 输出 10MHz      */
#define GPIO_MODE_OUT_2MHZ   0x2U   /* 输出 2MHz       */
#define GPIO_MODE_OUT_50MHZ  0x3U   /* 输出 50MHz      */

/* 常用组合: 通用推挽输出 @50MHz = 0x3 ; 通用推挽输出 @2MHz = 0x2 */
#define GPIO_OUT_PP_50MHZ    0x3U
#define GPIO_OUT_PP_2MHZ     0x2U

/* 引脚号掩码, 方便 BSRR/BRR/ODR/IDR 使用 */
#define GPIO_PIN_0    0x0001U
#define GPIO_PIN_1    0x0002U
#define GPIO_PIN_2    0x0004U
#define GPIO_PIN_3    0x0008U
#define GPIO_PIN_4    0x0010U
#define GPIO_PIN_5    0x0020U
#define GPIO_PIN_6    0x0040U
#define GPIO_PIN_7    0x0080U
#define GPIO_PIN_8    0x0100U
#define GPIO_PIN_9    0x0200U
#define GPIO_PIN_10   0x0400U
#define GPIO_PIN_11   0x0800U
#define GPIO_PIN_12   0x1000U
#define GPIO_PIN_13   0x2000U
#define GPIO_PIN_14   0x4000U
#define GPIO_PIN_15   0x8000U

#endif /* __STM32F103_REGS_H */
