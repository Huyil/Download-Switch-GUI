#ifndef ONE_I2CIO_H
#define ONE_I2CIO_H

#include <wire.h>
#include <arduino.h>

#define SLAVE_ADDR  (0x54>>1)  // 根据实际设置的从机地址修改
#define SLAVE_ADDR2 (0x56>>1)  // 根据实际设置的从机地址修改

#pragma pack(push, 1)  // 设置1字节对齐，防止填充
union BatteryRaw {
  uint16_t raw;
  struct {
    uint16_t  adc   : 10; // [9:0]   ADC 原始值 (0~1023)
    uint16_t  level : 4;  // [13:10] 电量等级 (0~15)
    uint16_t  chrg  : 1;  // [14]    是否正在充电
    uint16_t  done  : 1;  // [15]    是否充电完成
  };
};
#pragma pack(pop)       // 恢复对齐设置

void ioI2C_init(void);
void ioI2C_setChannelMode(uint8_t ch, uint8_t mode);
uint8_t ioI2C_readKeys(void);
BatteryRaw ioI2C_readBattery();

#endif // ONE_I2CIO_H