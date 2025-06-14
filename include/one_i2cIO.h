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
typedef struct {
  uint8_t mode : 3;
  uint8_t pin   : 2;
  uint8_t :3;
}io_one;

union IORaw{
    struct{
      uint32_t ch1 : 3;
      uint32_t io1 : 2;
      uint32_t dio : 3;
      
      uint32_t ch2 : 3;
      uint32_t io2 : 2;
      uint32_t clk : 3; 
      
      uint32_t ch3 : 3;
      uint32_t io3 : 2;
      uint32_t : 3;
      
      uint32_t ch4 : 3;
      uint32_t io4 : 2;
      uint32_t en_out:1;
      uint32_t en_5V:1;
    };
    
  struct {
    uint32_t  : 3;
    uint32_t P1 : 1;
    uint32_t N1 : 1;
    uint32_t DIO_EN : 1;
    uint32_t DIO_A : 1;
    uint32_t DIO_B : 1;
    
    uint32_t  : 3;
    uint32_t P2 : 1;
    uint32_t N2 : 1;
    uint32_t CLK_EN : 1;
    uint32_t CLK_A : 1;
    uint32_t CLK_B : 1;
    
    uint32_t  : 3;
    uint32_t P3 : 1;
    uint32_t N3 : 1;
    uint32_t : 3;
    
    uint32_t  : 3;
    uint32_t P4 : 1;
    uint32_t N4 : 1;
  }io;
  uint32_t ioset;
  io_one ch[4];
};
#pragma pack(pop)       // 恢复对齐设置

void ioI2C_init(void);
void ioI2C_setChannelMode(uint8_t ch, uint8_t mode);
uint8_t ioI2C_readKeys(void);
BatteryRaw ioI2C_readBattery();
IORaw ioI2C_readIO();
#endif // ONE_I2CIO_H