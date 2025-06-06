#include <Arduino.h>
#include "my_ota.hpp"
#include "my_wifi.hpp"
#include "one_i2cIO.h"
#include "one_gui.h"

void setup() {
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  delay(200);

  ioI2C_init();
  gui_init();

  my_wifi.ops->setSSID("dongriwifi2", "dongriwifi88736588");
  my_wifi.ops->connect();
  my_ota.ops->init();

}

unsigned long lastPollTime = 0;
const unsigned long pollInterval = 200; // 每 1 秒扫描一次
uint8_t keyValue = 0xFF, keyValueLast = 0xFF;
BatteryRaw bat,batLast;

static bool lastWifiConnected = false,blink = false;  // 记录上次状态
void loop() {
  unsigned long now = millis();
  my_ota.ops->loop();
  if (now - lastPollTime >= pollInterval) {
    lastPollTime = now;
    
    bool currentConnected = my_wifi.ops->isconnected();
    if (currentConnected != lastWifiConnected) {
        showWifiState(currentConnected);
        lastWifiConnected = currentConnected;
    }
    
    keyValue = ioI2C_readKeys();

    if(keyValue != keyValueLast)
    {
      keyValueLast = keyValue;
      showArrow(keyValue);
    }

    if(keyValue == 0xFF)
    {
      showLinkState(0);
      return;
    }else showLinkState(1);
    
    bat = ioI2C_readBattery();
    if(bat.chrg == 0)
    {
      blink = !blink;
      showBattery(bat.level?(blink?bat.level:bat.level-1):blink);
    }else if(batLast.level != bat.level)
    {
        showBattery(bat.level,bat.done==0);
        batLast.raw = bat.raw;
    }

    Serial.printf("[BAT] CHRG=%d, DONE=%d, LV=%d, ADC=%d\n",
            bat.chrg, bat.done,
            bat.level, bat.adc);

    // 处理按键输入
    if(gui.handleKeyInput(keyValue)) 
    // 遍历并绘制需要刷新的元素
    if (GUIElement::head) {
        GUIElement* current = GUIElement::head;
        do {
            if (current->isRefresh) {
                current->draw();
            }
            current = current->next;
        } while (current != GUIElement::head);
    }
  }
  delay(1);
}
