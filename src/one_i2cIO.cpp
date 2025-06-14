#include "one_i2cIO.h"

void ioI2C_init(void)
{
  Wire.begin(21,20,100000ul);  // 默认使用 GPIO 8 (SDA), GPIO 9 (SCL) for ESP32-C3
}


void ioI2C_setChannelMode(uint8_t ch, uint8_t mode) {
  if (ch > 3 || mode > 4) return; // 参数合法性检查

  uint8_t reg = 0x10 + ch; // 对应 0x10 ~ 0x13

  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(reg);     // 寄存器地址
  Wire.write(mode);    // 要写入的模式
  Wire.endTransmission();

  Serial.printf("CH%d 设置为 mode=%d\r\n", ch + 1, mode);
}


uint8_t ioI2C_readKeys() {
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(0x00); // key 寄存器
  Wire.endTransmission(false); // restart
  Wire.requestFrom(SLAVE_ADDR, 1);
  if (Wire.available()) {
    uint8_t key = Wire.read();
    return key;
  }
  return 0xFF;
}


BatteryRaw ioI2C_readBattery()
{
  BatteryRaw result = {0};

  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(0x01); // 读取电池状态地址
  Wire.endTransmission(false);
  Wire.requestFrom(SLAVE_ADDR, 2);

  if (Wire.available() >= 2) {
    uint8_t lo = Wire.read();
    uint8_t hi = Wire.read();
    result.raw = (hi << 8) | lo;

  }

  return result;
}

IORaw ioI2C_readIO()
{// Step 1: 读取 I2C 状态
    IORaw ioset;
    Wire.beginTransmission(SLAVE_ADDR);
    Wire.write(0x02); // 地址 0x02 读取 IO 状态
    Wire.endTransmission(false);
    Wire.requestFrom(SLAVE_ADDR, 4);
    if (Wire.available() == 4) {
        *(uint8_t*)&ioset.ch[0] = Wire.read();           // lo byte
        *(uint8_t*)&ioset.ch[1] = Wire.read();
        *(uint8_t*)&ioset.ch[2] = Wire.read();
        *(uint8_t*)&ioset.ch[3] = Wire.read();
    }

    return ioset;
}
