/*
 * aht20.h - AHT20 温湿度传感器驱动（uC/OS-II 版本）
 */
#ifndef AHT20_H
#define AHT20_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* AHT20 7 位地址 0x38，HAL 使用 8 位写地址 0x70 */
#define AHT20_I2C_ADDR      0x70U

/* 返回 0 成功，1 失败 */
uint8_t AHT20_Init(void);

/* 读取一次温湿度；返回 0 成功，1 失败。温度单位 ℃，湿度单位 %RH */
uint8_t AHT20_Read(float *temperature, float *humidity);

/* 最近一次读数是否有效 */
uint8_t AHT20_DataValid(void);

/* CRC 校验失败累计次数（调试用） */
uint8_t AHT20_CrcErrorCount(void);

#ifdef __cplusplus
}
#endif

#endif /* AHT20_H */
