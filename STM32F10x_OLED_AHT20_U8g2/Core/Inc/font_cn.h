/*
 * font_cn.h - 16x16 汉字点阵字库接口
 */
#ifndef FONT_CN_H
#define FONT_CN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 按 Unicode 码点查找 16x16 点阵，未找到返回 0 */
const uint8_t *FontCN_Get(uint16_t code);

/* 字库中汉字数量 */
uint16_t FontCN_Count(void);

#ifdef __cplusplus
}
#endif

#endif /* FONT_CN_H */
