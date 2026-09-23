/**
  ******************************************************************************
  * @file    main.c
  * @brief   STM32F103C8T6 最小系统板 —— GPIO 寄存器方式流水灯(跑马灯)
  *          使用 GPIOA / GPIOB / GPIOC 三个端口的引脚控制 LED, 轮流闪烁, 间隔 1 秒
  *
  * @note    【一】本工程是"纯寄存器"写法: 不使用 ST 标准外设库(StdPeriph_Lib)
  *               的任何函数, 所有操作都直接读写《STM32F10xxx 参考手册 RM0008》
  *               中定义的寄存器, 每个寄存器都标注了绝对地址, 便于对照手册。
  *          【二】系统时钟: 外部 8MHz 晶振(HSE) -> PLL 9 倍频 -> SYSCLK = 72MHz
  *               (若板上没有晶振或晶振损坏, 自动退回内部 HSI 8MHz 运行)
  *          【三】延时: 使用 Cortex-M3 内核的 SysTick 定时器(轮询方式, 不开中断),
  *               不用"空循环 for(i=0;i<1000000;i++)"这种与主频绑死的粗糙延时。
  *          【四】编译: Keil MDK 5.x (ARM Compiler 5 或 6 均可)
  *
  * @date    2026-09-22
  ******************************************************************************
  */

/* 只需要这一个头文件: 里面是自写的寄存器定义(不依赖 CMSIS / 标准库) */
#include "stm32f103_regs.h"


/* ==========================================================================
 *                         一、用户可修改的配置区
 * ========================================================================== */

/**
 * @brief 实验版本选择
 *        1 = 任务 2: 面包板上 3 只 LED(红 PA0 / 绿 PB0 / 蓝 PC14), 三灯轮流闪烁
 *        2 = 任务 3: 在任务 2 的基础上, 把板载 PC13 的 LED 也加入流水灯(共 4 灯)
 */
#define EXP_VERSION             2

/** 每只 LED 点亮的时间(毫秒), 题目要求"间隔 1 秒" */
#define LED_ON_TIME_MS          1000UL

/** LED 的公共结构体: 描述"接在哪个端口、哪个引脚、高电平还是低电平点亮" */
typedef struct
{
    GPIO_TypeDef *port;       /*!< 所属端口, 如 GPIOA               */
    uint8_t       pin;        /*!< 引脚号, 0 ~ 15                   */
    uint8_t       active_low; /*!< 0: 高电平点亮; 1: 低电平点亮     */
    const char   *name;       /*!< 名称, 仅用于阅读代码, 不参与运算 */
} LED_t;

/* --------------------------------------------------------------------------
 * 硬件接线表(与代码严格对应, 改线时只需改这张表)
 * --------------------------------------------------------------------------
 *  任务 2(EXP_VERSION = 1) 三只 LED:
 *      LED1 红  PA0   : PA0 --> 220Ω --> 红LED --> GND   高电平点亮
 *      LED2 绿  PB0   : PB0 --> 220Ω --> 绿LED --> GND   高电平点亮
 *      LED3 蓝  PC14  : PC14--> 1kΩ  --> 蓝LED --> GND   高电平点亮
 *  任务 3(EXP_VERSION = 2) 增加一只板载 LED:
 *      LED4 板载 PC13 : 板上已焊好(3.3V --> 电阻 --> LED --> PC13)  低电平点亮!
 *
 *  说明: 1) Blue Pill 只把 PC13 / PC14 / PC15 三根 PC 口引脚引到了排针上,
 *          其中 PC14 / PC15 与 32.768kHz 晶振(OSC32)复用、且驱动能力只有
 *          3mA, 所以外接 LED 的限流电阻要选大一点(1kΩ 左右)。
 *        2) 板载 PC13 的 LED 是"共阳接法": 阳极经电阻接 3.3V, 阴极接 PC13,
 *          因此 PC13 输出低电平时 LED 才亮, 这由结构体里的 active_low = 1 处理。
 *        3) 若不想使用 PC14, 可把蓝灯改接到 PC13 并同样采用"低电平点亮"接法
 *          (3.3V --> 电阻 --> LED --> PC13), 此时把 LED3 改成 {GPIOC, 13, 1, ...}
 *          即可; 注意 PC13 输出电流不要超过 3mA。
 * -------------------------------------------------------------------------- */
static LED_t s_leds[] =
{
    { GPIOA,  0, 0, "LED1 RED   on PA0 " },   /* 面包板 红色 LED  */
    { GPIOB,  0, 0, "LED2 GREEN on PB0 " },   /* 面包板 绿色 LED  */
    { GPIOC, 14, 0, "LED3 BLUE  on PC14" },   /* 面包板 蓝色 LED  */
    { GPIOC, 13, 1, "LED4 BOARD on PC13" },   /* 板载 LED(低电平点亮) */
};

/* 版本 1 只点亮前 3 只(不含板载 PC13), 版本 2 点亮全部 4 只 */
#if (EXP_VERSION == 1)
    #define LED_NUM   3U
#elif (EXP_VERSION == 2)
    #define LED_NUM   4U
#else
    #error "EXP_VERSION 只能是 1 或 2"
#endif


/* ==========================================================================
 *                     二、系统时钟(72MHz) 寄存器配置
 * ========================================================================== */

/** 全局变量: 保存当前 HCLK 频率(Hz), 供 SysTick 延时换算使用 */
static uint32_t g_hclk_hz;

/**
 * @brief  由寄存器反推当前系统时钟频率(Hz)
 * @note   读 RCC_CFGR 的 SWS 位判断当前用的是哪个时钟源, 再按 PLL 的
 *         "输入源 / 分频 / 倍频" 算出频率。这样即使外部晶振坏了、程序退回
 *         到内部 HSI 运行, 延时仍然是准的(这也是寄存器编程思路的体现)。
 */
static uint32_t Clock_GetHclkHz(void)
{
    uint32_t sws = RCC->CFGR & RCC_CFGR_SWS;   /* 取出 SWS[1:0] 状态位 */

    if (sws == RCC_CFGR_SWS_PLL)               /* 当前时钟源 = PLL */
    {
        uint32_t mul;      /* PLL 倍频系数 */
        uint32_t src;      /* PLL 输入频率 */

        /* PLLMULL[3:0] 的编码是: 0000=×2, 0001=×3, ..., 0111=×9, 1110/1111=×16 */
        mul = ((RCC->CFGR >> 18) & 0x0FUL) + 2UL;

        if (RCC->CFGR & RCC_CFGR_PLLSRC_HSE)   /* 输入源 = HSE */
        {
            /* PLLXTPRE=1 时先 2 分频 */
            src = (RCC->CFGR & RCC_CFGR_PLLXTPRE_HSE_DIV2) ? (8000000UL / 2UL)
                                                           : (8000000UL);
        }
        else                                   /* 输入源 = HSI/2 = 4MHz */
        {
            src = 4000000UL;
        }
        return src * mul;
    }
    else if (sws == RCC_CFGR_SWS_HSE)          /* HSE 直接做系统时钟 */
    {
        return 8000000UL;
    }
    else                                       /* HSI 8MHz 做系统时钟 */
    {
        return 8000000UL;
    }
}

/**
 * @brief  系统时钟初始化: HSE 8MHz -> PLL 9 倍频 -> 72MHz
 * @note   本函数由启动文件 startup_stm32f103_md.s 的 Reset_Handler 调用
 *         (在 C 库 __main 之前), 因此这里【只能操作硬件寄存器】, 绝对不要
 *         读写带初值的全局变量——那时 .data 段还没有从 Flash 拷贝到 RAM。
 *         配置步骤(RM0008 第 7 节 RCC):
 *           ① 开 HSE、等 HSERDY
 *           ② 设置 Flash 等待周期 = 2 个(AHB 72MHz 时必须)
 *           ③ 设置 AHB/APB1/APB2 分频: AHB=72M, APB2=72M, APB1=36M
 *           ④ 设置 PLL: 源=HSE, 9 倍频; 开 PLL、等 PLLRDY
 *           ⑤ 把 SYSCLK 切换到 PLL, 等 SWS=10
 */
void SystemInit(void)
{
    uint32_t timeout;

    /* ---------- ① 打开外部高速晶振 HSE 并等待稳定 ---------- */
    RCC->CR |= RCC_CR_HSEON;                        /* RCC_CR  = 0x40021000 */
    timeout = 0x20000UL;                            /* 简单超时, 防止晶振坏了死等 */
    while (((RCC->CR & RCC_CR_HSERDY) == 0UL) && (--timeout != 0UL))
    {
        /* 空等 */
    }

    if ((RCC->CR & RCC_CR_HSERDY) != 0UL)           /* HSE 就绪 -> 走 72MHz 方案 */
    {
        /* ---------- ② Flash 预取 + 2 个等待周期 ---------- */
        /*  ACR(0x40022000) = PRFTBE(bit4) | LATENCY(bit2:0)=0b010  */
        FLASH_ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

        /* ---------- ③ 总线分频 ---------- */
        /*  CFGR(0x40021004): HPRE=A, PPRE2=A, PPRE1=HCLK/2(APB1 最高 36MHz) */
        RCC->CFGR = RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE2_DIV1 | RCC_CFGR_PPRE1_DIV2;

        /* ---------- ④ PLL: 时钟源 = HSE, 倍频 = 9 (8MHz × 9 = 72MHz) ---------- */
        RCC->CFGR |= (RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9);
        RCC->CR   |= RCC_CR_PLLON;                  /* 打开 PLL(此时 PLL 还没做系统时钟) */
        while ((RCC->CR & RCC_CR_PLLRDY) == 0UL)    /* 等待 PLL 锁定 */
        {
            /* 空等 */
        }

        /* ---------- ⑤ 切换系统时钟源: HSI -> PLL ---------- */
        RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
        while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
        {
            /* 空等, 直到状态位显示 PLL 已成为系统时钟 */
        }
    }
    else                                            /* 没有晶振 -> 退回内部 HSI 8MHz */
    {
        RCC->CR |= RCC_CR_HSION;
        while ((RCC->CR & RCC_CR_HSIRDY) == 0UL)
        {
            /* 空等 */
        }
        FLASH_ACR = FLASH_ACR_PRFTBE;               /* 8MHz 不需要等待周期 */
        RCC->CFGR &= ~RCC_CFGR_SW;                  /* SW = 00: 选择 HSI */
    }
}


/* ==========================================================================
 *                   三、SysTick(内核定时器) 毫秒级延时
 * ========================================================================== */

/**
 * @brief  毫秒级阻塞延时(轮询方式, 不使用中断)
 * @param  ms 延时长度, 单位毫秒
 * @note   SysTick 是 Cortex-M3 内核自带的 24 位递减计数器, 寄存器地址:
 *           0xE000E010  CTRL  : bit0 使能, bit1 中断允许, bit2 时钟源(1=HCLK),
 *                               bit16 计数到 0 的标志 COUNTFLAG(读后自动清 0)
 *           0xE000E014  LOAD  : 重装载值, 计数到这里后回到 0 并置 COUNTFLAG
 *           0xE000E018  VAL   : 当前计数值(写任何值都会清 0)
 *         每 1ms 需要计数 (HCLK / 1000) 次, 例如 72MHz 时为 72000 次。
 */
static void SysTick_DelayMs(uint32_t ms)
{
    uint32_t reload = (g_hclk_hz / 1000UL) - 1UL;   /* 72MHz -> 72000 - 1 */

    while (ms != 0UL)
    {
        SysTick->LOAD = reload;                     /* 设置重装载值          */
        SysTick->VAL  = 0UL;                        /* 清当前计数器          */
        SysTick->CTRL = SYSTICK_CTRL_CLKSOURCE      /* 时钟源 = HCLK         */
                      | SYSTICK_CTRL_ENABLE;        /* 使能(不产生中断)      */

        /* COUNTFLAG(bit16) = 1 说明计数了一次 0, 即已经过了 1ms */
        while ((SysTick->CTRL & SYSTICK_CTRL_COUNTFLAG) == 0UL)
        {
            /* 空等 1ms */
        }

        SysTick->CTRL = 0UL;                        /* 关闭 SysTick, 下次重新装载 */
        ms--;
    }
}


/* ==========================================================================
 *                       四、GPIO 寄存器操作函数
 * ========================================================================== */

/**
 * @brief  使能某个 GPIO 端口的时钟
 * @param  port GPIOA / GPIOB / GPIOC
 * @note   RCC_APB2ENR 地址 = 0x40021018
 *           bit2 = IOPAEN(GPIOA), bit3 = IOPBEN(GPIOB), bit4 = IOPCEN(GPIOC)
 *         STM32 的外设"默认不给时钟", 不使能时钟就去配置引脚是无效的,
 *         这是初学者最容易漏掉、也最难查的一步!
 */
static void GPIO_ClockEnable(GPIO_TypeDef *port)
{
    if (port == GPIOA)
    {
        RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    }
    else if (port == GPIOB)
    {
        RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    }
    else if (port == GPIOC)
    {
        RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    }
    else
    {
        /* 本实验不使用 GPIOD/GPIOE */
    }

    /* 回读一次, 保证"时钟使能"这个写操作已经真正到达外设(ARM 的写操作是带缓冲的) */
    (void)RCC->APB2ENR;
}

/**
 * @brief  把一个引脚配置成"通用推挽输出"
 * @param  port 端口指针, 如 GPIOA
 * @param  pin  引脚号 0~15
 * @param  mode_cnf 4 位配置字 = [CNF1 CNF0 MODE1 MODE0],
 *                  例如 0x3 = 通用推挽输出@50MHz, 0x2 = 通用推挽输出@2MHz
 *
 * @note   CRL(0x00) 管 Px0~Px7 , 每个引脚占 4 位: 引脚 n -> bit[4n+3 : 4n]
 *         CRH(0x04) 管 Px8~Px15, 每个引脚占 4 位: 引脚 n -> bit[4(n-8)+3 : 4(n-8)]
 *         复位后 CRL/CRH = 0x44444444, 即所有引脚都是"浮空输入"。
 *         写法: 先读出原值, 用 &~ 清掉这 4 位, 再用 | 写入新值,
 *               这样不会影响同一个寄存器里其它引脚的配置。
 */
static void GPIO_ConfigOutput(GPIO_TypeDef *port, uint8_t pin, uint32_t mode_cnf)
{
    __IO uint32_t *cr;        /* 指向 CRL 或 CRH */
    uint32_t       shift;     /* 该引脚在寄存器中的起始位 */

    if (pin < 8U)
    {
        cr    = &port->CRL;                /* 低 8 个引脚用 CRL */
        shift = (uint32_t)pin * 4UL;
    }
    else
    {
        cr    = &port->CRH;                /* 高 8 个引脚用 CRH */
        shift = ((uint32_t)pin - 8UL) * 4UL;
    }

    *cr = (*cr & ~(0xFUL << shift)) | ((mode_cnf & 0xFUL) << shift);
}

/**
 * @brief  把一个引脚配置成输入(RM0008 中 GPIOC 输入模式的配置字)
 * @note   输入模式同样使用每引脚 4 位的 [CNF1 CNF0 MODE1 MODE0],
 *         此时 MODE[1:0] = 00, 由 CNF 决定输入方式:
 *           0x4 = 0100 浮空输入(复位默认值)
 *           0x8 = 1000 上拉/下拉输入(上拉还是下拉由 ODR 对应位决定: 1 = 上拉)
 *           0x0 = 0000 模拟输入(ADC 用)
 *         本工程只输出点灯, 不需要输入, 故未编写输入配置函数;
 *         若以后要读按键, 按上面的数值调用 GPIO_ConfigOutput() 即可
 *         (位布局与输出完全一样, 只是 4 位参数的含义不同)。
 */
/**
 * @brief  写引脚电平(使用 BSRR, 一条写指令完成, 是"原子操作")
 * @param  port 端口指针; pin 引脚号; level 1 = 高电平, 0 = 低电平
 * @note   BSRR 地址 = 端口基址 + 0x10:
 *           低 16 位写 1 -> 对应引脚输出高电平(置位)
 *           高 16 位写 1 -> 对应引脚输出低电平(复位)
 *         写 0 的位不受影响, 所以只影响目标引脚, 且【不需要先读后写】,
 *         在中断里也能安全使用(对比: 直接改 ODR 需要 读-改-写 三步)。
 */
static void GPIO_WritePin(GPIO_TypeDef *port, uint8_t pin, uint32_t level)
{
    if (level != 0UL)
    {
        port->BSRR = (1UL << pin);              /* bit n   = 1 -> Pxn 输出 1 */
    }
    else
    {
        port->BSRR = (1UL << (pin + 16UL));     /* bit n+16 = 1 -> Pxn 输出 0 */
    }
}

/**
 * @brief  翻转引脚电平
 * @note   本工程用不到翻转; 需要时按"读出 ODR -> 取反 -> 写 BSRR"的思路即可,
 *         STM32F1 没有 F4 那样的 BSRR 高低位同时写 1 的翻转方式。
 */


/* ==========================================================================
 *                         五、LED 驱动函数
 * ========================================================================== */

/* LED 操作函数声明(先声明后使用, 避免 C99 的隐式声明错误) */
static void LED_On(const LED_t *led);
static void LED_Off(const LED_t *led);

/**
 * @brief  初始化所有 LED 引脚: 使能时钟 + 配置成通用推挽输出
 * @note   面包板上的 LED 用 50MHz 推挽输出(驱动能力强, 波形上升沿陡);
 *         板载 PC13 用 2MHz 即可(它连的是板上 LED, 速度要求低, 还能减小干扰)。
 */
static void LED_Init(void)
{
    uint8_t i;

    for (i = 0U; i < (uint8_t)(sizeof(s_leds) / sizeof(s_leds[0])); i++)
    {
        GPIO_ClockEnable(s_leds[i].port);                 /* ① 先开端口时钟 */

        if (s_leds[i].port == GPIOC && s_leds[i].pin >= 13U)
        {
            /* PC13/PC14/PC15 接的是板上/晶振网络, 速度选 2MHz 更稳妥 */
            GPIO_ConfigOutput(s_leds[i].port, s_leds[i].pin, GPIO_OUT_PP_2MHZ);
        }
        else
        {
            GPIO_ConfigOutput(s_leds[i].port, s_leds[i].pin, GPIO_OUT_PP_50MHZ);
        }

        LED_Off(&s_leds[i]);                              /* ② 上电先全部熄灭 */
    }
}

/**
 * @brief  点亮 LED(自动处理"高电平点亮"与"低电平点亮"两种情况)
 */
static void LED_On(const LED_t *led)
{
    GPIO_WritePin(led->port, led->pin, (led->active_low != 0U) ? 0UL : 1UL);
}

/**
 * @brief  熄灭 LED
 */
static void LED_Off(const LED_t *led)
{
    GPIO_WritePin(led->port, led->pin, (led->active_low != 0U) ? 1UL : 0UL);
}


/* ==========================================================================
 *                            六、主函数
 * ========================================================================== */

/**
 * @brief  程序入口
 * @note   执行顺序: 上电 -> 取向量表第 1 个字装入 MSP -> Reset_Handler
 *         -> SystemInit()(配时钟) -> __main(C 库初始化) -> main()
 */
int main(void)
{
    uint8_t i;

    /* ---------- 第 1 步: 记录当前系统时钟频率(72MHz), 供延时换算 ---------- */
    g_hclk_hz = Clock_GetHclkHz();

    /* ---------- 第 2 步: 把 PA0 / PB0 / PC14 (以及 PC13) 配成推挽输出 ---------- */
    LED_Init();

    /* ---------- 第 3 步: 无限循环, 逐个点亮 LED, 每个亮 LED_ON_TIME_MS 毫秒 ---------- */
    for (;;)
    {
        for (i = 0U; i < LED_NUM; i++)
        {
            LED_On(&s_leds[i]);                     /* 点亮第 i 只 LED     */
            SysTick_DelayMs(LED_ON_TIME_MS);        /* 保持 1000ms         */
            LED_Off(&s_leds[i]);                    /* 熄灭第 i 只 LED     */
        }
        /* 4 只灯走完一轮 = 4 秒, 然后从头再来, 形成"流水灯"效果 */
    }
}

/************************ (C) COPYRIGHT *****END OF FILE***********************/
