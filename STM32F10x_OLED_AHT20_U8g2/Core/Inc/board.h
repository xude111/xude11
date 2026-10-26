/*
 * board.h - 板级服务：LED 心跳、按键扫描、非阻塞延时、串口调试
 */
#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Board_Init(void);
void Board_Service(void);
uint8_t Board_KeyPressed(void);
void Board_Delay(uint32_t ms);
void Board_Print(const char *s);
void Board_Printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
