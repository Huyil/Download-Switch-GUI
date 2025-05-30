#include <Arduino.h>
#include "one_gfx.h"
#include "one_i2cIO.h"
#include "my_ota.hpp"
#include "my_wifi.hpp"

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(1000);

  ioI2C_init();
  lcd_init();

  my_wifi.ops->setSSID("dongriwifi2", "dongriwifi88736588");
  my_wifi.ops->connect();
  my_ota.ops->init();
}

unsigned long lastPollTime = 0;
const unsigned long pollInterval = 100; // 每 1 秒扫描一次
uint8_t keyValue = 0xFF, keyValueLast = 0xFF;
BatteryRaw bat,batLast;
void loop() {
  unsigned long now = millis();
  my_ota.ops->loop();
  if (now - lastPollTime >= pollInterval) {
    lastPollTime = now;

    keyValue = ioI2C_readKeys();
    bat = ioI2C_readBattery();

    if(keyValue != keyValueLast)
    {
      keyValueLast = keyValue;
      showArrow(keyValue);
    }

    if(batLast.level != bat.level)
    {
      if(batLast.level != bat.level)
      {
        showBattery(bat.level);
        
        Serial.printf("[BAT] CHRG=%d, DONE=%d, LV=%d, ADC=%d\n",
            bat.chrg, bat.done,
            bat.level, bat.adc);
      }
      if(bat.raw == 0 || bat.raw == 0)
      {
        showLinkState(bat.raw?1:0);
      }
      batLast.raw = bat.raw;
    }
  }
  delay(1);
}
