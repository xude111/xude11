;*******************************************************************************
;* 文件名  : startup_stm32f103_md.s
;* 说明    : STM32F103C8T6(中容量, Medium Density) 启动文件 —— 手写精简版
;* 功能    : 1) 定义栈顶指针 __initial_sp 与堆 __heap_base
;*           2) 定义中断向量表(复位后从 0x08000000 处取第 1、2 个字)
;*           3) 复位入口 Reset_Handler: 先调用 SystemInit() 配置时钟,
;*              再跳转到 C 库的 __main 完成变量初始化, 最后进入 main()
;* 编译    : Keil MDK 的 armasm 汇编器(AC5 / AC6 均可)
;* 备选    : 也可以直接用 Keil 安装目录里的官方启动文件, 二者功能相同, 路径形如:
;*           [Keil安装目录] / ARM / PACK / Keil / STM32F1xx_DFP / [版本] /
;*           Device / Source / ARM / startup_stm32f10x_md.s
;*******************************************************************************

; <h> 栈空间(Stack)设置: 1KB, 按 8 字节对齐
Stack_Size      EQU     0x00000400

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp

; <h> 堆空间(Heap)设置: 512 字节(本工程不使用 malloc, 保留以便将来扩展)
Heap_Size       EQU     0x00000200

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit

                PRESERVE8
                THUMB

;*******************************************************************************
; 中断向量表: 必须放在 Flash 的最前面(0x08000000), 上电后内核从这里取 MSP 和 PC
;*******************************************************************************
                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

__Vectors       DCD     __initial_sp                ; 0x00 栈顶地址
                DCD     Reset_Handler               ; 0x04 复位入口
                DCD     NMI_Handler                 ; 0x08 不可屏蔽中断
                DCD     HardFault_Handler           ; 0x0C 硬件错误
                DCD     MemManage_Handler           ; 0x10 存储器管理错误
                DCD     BusFault_Handler            ; 0x14 总线错误
                DCD     UsageFault_Handler          ; 0x18 用法错误
                DCD     0                           ; 0x1C 保留
                DCD     0                           ; 0x20 保留
                DCD     0                           ; 0x24 保留
                DCD     0                           ; 0x28 保留
                DCD     SVC_Handler                 ; 0x2C 系统服务调用
                DCD     DebugMon_Handler            ; 0x30 调试监视
                DCD     0                           ; 0x34 保留
                DCD     PendSV_Handler              ; 0x38 可挂起系统服务
                DCD     SysTick_Handler             ; 0x3C 系统嘀嗒定时器
                ; ------- 以下为 STM32F103 外设中断(中容量共 43 个) -------
                DCD     WWDG_IRQHandler             ; 0
                DCD     PVD_IRQHandler              ; 1
                DCD     TAMPER_IRQHandler           ; 2
                DCD     RTC_IRQHandler              ; 3
                DCD     FLASH_IRQHandler            ; 4
                DCD     RCC_IRQHandler              ; 5
                DCD     EXTI0_IRQHandler            ; 6
                DCD     EXTI1_IRQHandler            ; 7
                DCD     EXTI2_IRQHandler            ; 8
                DCD     EXTI3_IRQHandler            ; 9
                DCD     EXTI4_IRQHandler            ; 10
                DCD     DMA1_Channel1_IRQHandler    ; 11
                DCD     DMA1_Channel2_IRQHandler    ; 12
                DCD     DMA1_Channel3_IRQHandler    ; 13
                DCD     DMA1_Channel4_IRQHandler    ; 14
                DCD     DMA1_Channel5_IRQHandler    ; 15
                DCD     DMA1_Channel6_IRQHandler    ; 16
                DCD     DMA1_Channel7_IRQHandler    ; 17
                DCD     ADC1_2_IRQHandler           ; 18
                DCD     USB_HP_CAN1_TX_IRQHandler   ; 19
                DCD     USB_LP_CAN1_RX0_IRQHandler  ; 20
                DCD     CAN1_RX1_IRQHandler         ; 21
                DCD     CAN1_SCE_IRQHandler         ; 22
                DCD     EXTI9_5_IRQHandler          ; 23
                DCD     TIM1_BRK_IRQHandler         ; 24
                DCD     TIM1_UP_IRQHandler          ; 25
                DCD     TIM1_TRG_COM_IRQHandler     ; 26
                DCD     TIM1_CC_IRQHandler          ; 27
                DCD     TIM2_IRQHandler             ; 28
                DCD     TIM3_IRQHandler             ; 29
                DCD     TIM4_IRQHandler             ; 30
                DCD     I2C1_EV_IRQHandler          ; 31
                DCD     I2C1_ER_IRQHandler          ; 32
                DCD     I2C2_EV_IRQHandler          ; 33
                DCD     I2C2_ER_IRQHandler          ; 34
                DCD     SPI1_IRQHandler             ; 35
                DCD     SPI2_IRQHandler             ; 36
                DCD     USART1_IRQHandler           ; 37
                DCD     USART2_IRQHandler           ; 38
                DCD     USART3_IRQHandler           ; 39
                DCD     EXTI15_10_IRQHandler        ; 40
                DCD     RTCAlarm_IRQHandler         ; 41
                DCD     USBWakeUp_IRQHandler        ; 42
                DCD     TIM8_BRK_IRQHandler         ; 43 (大容量芯片才有, 此处保留占位)
                DCD     TIM8_UP_IRQHandler          ; 44
                DCD     TIM8_TRG_COM_IRQHandler     ; 45
                DCD     TIM8_CC_IRQHandler          ; 46
                DCD     ADC3_IRQHandler             ; 47
                DCD     FSMC_IRQHandler             ; 48
                DCD     SDIO_IRQHandler             ; 49
                DCD     TIM5_IRQHandler             ; 50
                DCD     SPI3_IRQHandler             ; 51
                DCD     UART4_IRQHandler            ; 52
                DCD     UART5_IRQHandler            ; 53
                DCD     TIM6_IRQHandler             ; 54
                DCD     TIM7_IRQHandler             ; 55
                DCD     DMA2_Channel1_IRQHandler    ; 56
                DCD     DMA2_Channel2_IRQHandler    ; 57
                DCD     DMA2_Channel3_IRQHandler    ; 58
                DCD     DMA2_Channel4_5_IRQHandler  ; 59
__Vectors_End

__Vectors_Size  EQU     __Vectors_End - __Vectors

;*******************************************************************************
; 代码段: 复位处理程序
;*******************************************************************************
                AREA    |.text|, CODE, READONLY

Reset_Handler   PROC
                EXPORT  Reset_Handler               [WEAK]
                IMPORT  SystemInit                  ; 在 main.c 中实现(配置 72MHz 时钟)
                IMPORT  __main                      ; C 库入口: 初始化变量后调用 main()
                LDR     R0, =SystemInit
                BLX     R0
                LDR     R0, =__main
                BX      R0
                ENDP

;*******************************************************************************
; 默认中断服务程序(弱定义): 所有未使用的中断都停在这里, 便于调试
;   说明: 若某个中断被使能却没有编写处理函数, 程序会死在此处 -> 用调试器很容易发现
;*******************************************************************************
Default_Handler PROC
                EXPORT  NMI_Handler                 [WEAK]
                EXPORT  HardFault_Handler           [WEAK]
                EXPORT  MemManage_Handler           [WEAK]
                EXPORT  BusFault_Handler            [WEAK]
                EXPORT  UsageFault_Handler          [WEAK]
                EXPORT  SVC_Handler                 [WEAK]
                EXPORT  DebugMon_Handler            [WEAK]
                EXPORT  PendSV_Handler              [WEAK]
                EXPORT  SysTick_Handler             [WEAK]
                EXPORT  WWDG_IRQHandler             [WEAK]
                EXPORT  PVD_IRQHandler              [WEAK]
                EXPORT  TAMPER_IRQHandler           [WEAK]
                EXPORT  RTC_IRQHandler              [WEAK]
                EXPORT  FLASH_IRQHandler            [WEAK]
                EXPORT  RCC_IRQHandler              [WEAK]
                EXPORT  EXTI0_IRQHandler            [WEAK]
                EXPORT  EXTI1_IRQHandler            [WEAK]
                EXPORT  EXTI2_IRQHandler            [WEAK]
                EXPORT  EXTI3_IRQHandler            [WEAK]
                EXPORT  EXTI4_IRQHandler            [WEAK]
                EXPORT  DMA1_Channel1_IRQHandler    [WEAK]
                EXPORT  DMA1_Channel2_IRQHandler    [WEAK]
                EXPORT  DMA1_Channel3_IRQHandler    [WEAK]
                EXPORT  DMA1_Channel4_IRQHandler    [WEAK]
                EXPORT  DMA1_Channel5_IRQHandler    [WEAK]
                EXPORT  DMA1_Channel6_IRQHandler    [WEAK]
                EXPORT  DMA1_Channel7_IRQHandler    [WEAK]
                EXPORT  ADC1_2_IRQHandler           [WEAK]
                EXPORT  USB_HP_CAN1_TX_IRQHandler   [WEAK]
                EXPORT  USB_LP_CAN1_RX0_IRQHandler  [WEAK]
                EXPORT  CAN1_RX1_IRQHandler         [WEAK]
                EXPORT  CAN1_SCE_IRQHandler         [WEAK]
                EXPORT  EXTI9_5_IRQHandler          [WEAK]
                EXPORT  TIM1_BRK_IRQHandler         [WEAK]
                EXPORT  TIM1_UP_IRQHandler          [WEAK]
                EXPORT  TIM1_TRG_COM_IRQHandler     [WEAK]
                EXPORT  TIM1_CC_IRQHandler          [WEAK]
                EXPORT  TIM2_IRQHandler             [WEAK]
                EXPORT  TIM3_IRQHandler             [WEAK]
                EXPORT  TIM4_IRQHandler             [WEAK]
                EXPORT  I2C1_EV_IRQHandler          [WEAK]
                EXPORT  I2C1_ER_IRQHandler          [WEAK]
                EXPORT  I2C2_EV_IRQHandler          [WEAK]
                EXPORT  I2C2_ER_IRQHandler          [WEAK]
                EXPORT  SPI1_IRQHandler             [WEAK]
                EXPORT  SPI2_IRQHandler             [WEAK]
                EXPORT  USART1_IRQHandler           [WEAK]
                EXPORT  USART2_IRQHandler           [WEAK]
                EXPORT  USART3_IRQHandler           [WEAK]
                EXPORT  EXTI15_10_IRQHandler        [WEAK]
                EXPORT  RTCAlarm_IRQHandler         [WEAK]
                EXPORT  USBWakeUp_IRQHandler        [WEAK]
                EXPORT  TIM8_BRK_IRQHandler         [WEAK]
                EXPORT  TIM8_UP_IRQHandler          [WEAK]
                EXPORT  TIM8_TRG_COM_IRQHandler     [WEAK]
                EXPORT  TIM8_CC_IRQHandler          [WEAK]
                EXPORT  ADC3_IRQHandler             [WEAK]
                EXPORT  FSMC_IRQHandler             [WEAK]
                EXPORT  SDIO_IRQHandler             [WEAK]
                EXPORT  TIM5_IRQHandler             [WEAK]
                EXPORT  SPI3_IRQHandler             [WEAK]
                EXPORT  UART4_IRQHandler            [WEAK]
                EXPORT  UART5_IRQHandler            [WEAK]
                EXPORT  TIM6_IRQHandler             [WEAK]
                EXPORT  TIM7_IRQHandler             [WEAK]
                EXPORT  DMA2_Channel1_IRQHandler    [WEAK]
                EXPORT  DMA2_Channel2_IRQHandler    [WEAK]
                EXPORT  DMA2_Channel3_IRQHandler    [WEAK]
                EXPORT  DMA2_Channel4_5_IRQHandler  [WEAK]

NMI_Handler
HardFault_Handler
MemManage_Handler
BusFault_Handler
UsageFault_Handler
SVC_Handler
DebugMon_Handler
PendSV_Handler
SysTick_Handler
WWDG_IRQHandler
PVD_IRQHandler
TAMPER_IRQHandler
RTC_IRQHandler
FLASH_IRQHandler
RCC_IRQHandler
EXTI0_IRQHandler
EXTI1_IRQHandler
EXTI2_IRQHandler
EXTI3_IRQHandler
EXTI4_IRQHandler
DMA1_Channel1_IRQHandler
DMA1_Channel2_IRQHandler
DMA1_Channel3_IRQHandler
DMA1_Channel4_IRQHandler
DMA1_Channel5_IRQHandler
DMA1_Channel6_IRQHandler
DMA1_Channel7_IRQHandler
ADC1_2_IRQHandler
USB_HP_CAN1_TX_IRQHandler
USB_LP_CAN1_RX0_IRQHandler
CAN1_RX1_IRQHandler
CAN1_SCE_IRQHandler
EXTI9_5_IRQHandler
TIM1_BRK_IRQHandler
TIM1_UP_IRQHandler
TIM1_TRG_COM_IRQHandler
TIM1_CC_IRQHandler
TIM2_IRQHandler
TIM3_IRQHandler
TIM4_IRQHandler
I2C1_EV_IRQHandler
I2C1_ER_IRQHandler
I2C2_EV_IRQHandler
I2C2_ER_IRQHandler
SPI1_IRQHandler
SPI2_IRQHandler
USART1_IRQHandler
USART2_IRQHandler
USART3_IRQHandler
EXTI15_10_IRQHandler
RTCAlarm_IRQHandler
USBWakeUp_IRQHandler
TIM8_BRK_IRQHandler
TIM8_UP_IRQHandler
TIM8_TRG_COM_IRQHandler
TIM8_CC_IRQHandler
ADC3_IRQHandler
FSMC_IRQHandler
SDIO_IRQHandler
TIM5_IRQHandler
SPI3_IRQHandler
UART4_IRQHandler
UART5_IRQHandler
TIM6_IRQHandler
TIM7_IRQHandler
DMA2_Channel1_IRQHandler
DMA2_Channel2_IRQHandler
DMA2_Channel3_IRQHandler
DMA2_Channel4_5_IRQHandler
                B       .                           ; 原地死循环, 等待调试器
                ENDP

                ALIGN

;*******************************************************************************
; 堆/栈初始化: 使用 Keil MicroLIB 时由库自己处理, 否则需要提供 __user_initial_stackheap
;*******************************************************************************
                IF      :DEF:__MICROLIB
                EXPORT  __initial_sp
                EXPORT  __heap_base
                EXPORT  __heap_limit
                ELSE
                IMPORT  __use_two_region_memory
                EXPORT  __user_initial_stackheap
__user_initial_stackheap
                LDR     R0, =  Heap_Mem
                LDR     R1, =(Stack_Mem + Stack_Size)
                LDR     R2, = (Heap_Mem +  Heap_Size)
                LDR     R3, = Stack_Mem
                BX      LR
                ALIGN
                ENDIF

                END
