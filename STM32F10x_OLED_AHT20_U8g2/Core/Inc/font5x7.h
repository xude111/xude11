/*
 * font5x7.h - 5x7 ASCII 点阵字库接口
 */
#ifndef FONT5X7_H
#define FONT5X7_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 返回 5 列点阵指针；不可打印字符按空格处理 */
const uint8_t *Font5x7_Get(char c);

#ifdef __cplusplus
}
#endif

#endif /* FONT5X7_H */
