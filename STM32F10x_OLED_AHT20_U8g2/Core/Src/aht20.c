/*
 * aht20.c - AHT20 温湿度传感器驱动（I2C，阻塞方式）
 *
 * 参考 AHT20 数据手册：
 *   上电等待 40ms
 *   初始化命令 0xBE 0x08 0x00，等待 10ms
 *   触发测量   0xAC 0x33 0x00，等待 80ms
 *   读取 7 字节：状态 + 湿度20bit + 温度20bit + CRC
 */
#include "main.h"
#include "aht20.h"
#include "app_config.h"

#define AHT20_CMD_INIT      0xBEU
#define AHT20_CMD_TRIGGER   0xACU
#define AHT20_STATUS_BUSY   0x80U
#define AHT20_STATUS_CAL    0x08U

static uint8_t s_valid = 0;
static uint8_t s_ready = 0;
static uint8_t s_crc_errors = 0;

static uint8_t AHT20_Crc8(const uint8_t *data, uint8_t len)
{
  uint8_t crc = 0xFFU;
  uint8_t i, bit;

  for (i = 0; i < len; i++)
  {
    crc ^= data[i];
    for (bit = 0; bit < 8; bit++)
    {
      if (crc & 0x80U)
      {
        crc = (uint8_t)((crc << 1) ^ 0x31U);
      }
      else
      {
        crc = (uint8_t)(crc << 1);
      }
    }
  }
  return crc;
}

uint8_t AHT20_Init(void)
{
  uint8_t status = 0;
  uint8_t cmd[3];

  s_ready = 0;
  s_valid = 0;

  if (HAL_I2C_IsDeviceReady(&hi2c1, AHT20_I2C_ADDR, 3, 100) != HAL_OK)
  {
    return 1;
  }

  HAL_Delay(40);

  if (HAL_I2C_Master_Receive(&hi2c1, AHT20_I2C_ADDR, &status, 1, 100) != HAL_OK)
  {
    return 1;
  }

  /* 状态位 bit3=1 表示已校准；否则发送初始化命令 */
  if ((status & AHT20_STATUS_CAL) == 0U)
  {
    cmd[0] = AHT20_CMD_INIT;
    cmd[1] = 0x08U;
    cmd[2] = 0x00U;
    if (HAL_I2C_Master_Transmit(&hi2c1, AHT20_I2C_ADDR, cmd, 3, 100) != HAL_OK)
    {
      return 1;
    }
    HAL_Delay(10);
  }

  if (HAL_I2C_Master_Receive(&hi2c1, AHT20_I2C_ADDR, &status, 1, 100) != HAL_OK)
  {
    return 1;
  }

  s_ready = 1;
  return ((status & AHT20_STATUS_CAL) != 0U) ? 0U : 1U;
}

uint8_t AHT20_Read(float *temperature, float *humidity)
{
  uint8_t cmd[3] = {AHT20_CMD_TRIGGER, 0x33U, 0x00U};
  uint8_t buf[7];
  uint32_t raw_h, raw_t;
  uint8_t retry;

  if (temperature == 0 || humidity == 0)
  {
    return 1;
  }

  if (!s_ready)
  {
    if (AHT20_Init() != 0U)
    {
      s_valid = 0;
      return 1;
    }
  }

  for (retry = 0; retry < 3U; retry++)
  {
    if (HAL_I2C_Master_Transmit(&hi2c1, AHT20_I2C_ADDR, cmd, 3, 100) != HAL_OK)
    {
      s_valid = 0;
      HAL_Delay(10);
      continue;
    }

    HAL_Delay(80);

    if (HAL_I2C_Master_Receive(&hi2c1, AHT20_I2C_ADDR, buf, 7, 100) != HAL_OK)
    {
      s_valid = 0;
      continue;
    }

    if (buf[0] & AHT20_STATUS_BUSY)
    {
      HAL_Delay(20);
      continue;
    }

    /* CRC 校验：若失败只记录，不丢弃数据（不同批次模块 CRC 实现有差异） */
    if (AHT20_Crc8(buf, 6) != buf[6])
    {
      s_crc_errors++;
    }

    raw_h = ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | (uint32_t)buf[3];
    raw_h >>= 4;
    raw_t = (((uint32_t)(buf[3] & 0x0FU)) << 16)
          | ((uint32_t)buf[4] << 8)
          | (uint32_t)buf[5];

    *humidity = ((float)raw_h * 100.0f) / 1048576.0f;
    *temperature = ((float)raw_t * 200.0f) / 1048576.0f - 50.0f;

    if (*humidity < 0.0f)   { *humidity = 0.0f; }
    if (*humidity > 100.0f) { *humidity = 100.0f; }
    if (*temperature < -40.0f) { *temperature = -40.0f; }
    if (*temperature > 85.0f)  { *temperature = 85.0f; }

    s_valid = 1;
    return 0;
  }

  s_valid = 0;
  return 1;
}

uint8_t AHT20_DataValid(void)
{
  return s_valid;
}

uint8_t AHT20_CrcErrorCount(void)
{
  return s_crc_errors;
}
