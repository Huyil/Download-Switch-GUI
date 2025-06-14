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

unsigned long lastKeyPollTime = 0;
unsigned long lastKeyActionTime = 0;
const unsigned long keyPollInterval = 10;
const unsigned long keyRepeatInterval = 200;

uint8_t keyValue = 0xFF, keyValueLast = 0xFF;
BatteryRaw bat, batLast;

static bool lastWifiConnected = false, blink = false;

void loop() {
  unsigned long now = millis();
  my_ota.ops->loop();

  // --- 按键处理部分，每10ms ---
  if (now - lastKeyPollTime >= keyPollInterval) {
    lastKeyPollTime = now;

    keyValue = ioI2C_readKeys();

    if (keyValue != 0xFF) {
      if ((keyValue&0x1F) != keyValueLast) {
        // 按键变化：立即执行
        keyValueLast = keyValue&0x1F;
        gui.handleKeyInput(keyValue);
        //showArrow(keyValue);
        lastKeyActionTime = now;
      } else if (now - lastKeyActionTime >= keyRepeatInterval) {
        // 按键相同，150ms重复触发
        gui.handleKeyInput(keyValue);
        lastKeyActionTime = now;
      }

      showLinkState(1);
    } else {
      // 按键释放
      keyValueLast = 0x1F;
      showLinkState(0);
    }
  }

  // --- 状态刷新部分，每200ms ---
  static unsigned long lastStatusPollTime = 0;
  const unsigned long statusPollInterval = 200;

  if (now - lastStatusPollTime >= statusPollInterval) {
    lastStatusPollTime = now;

    // WiFi 状态变化
    bool currentConnected = my_wifi.ops->isconnected();
    if (currentConnected != lastWifiConnected) {
      showWifiState(currentConnected);
      lastWifiConnected = currentConnected;
    }

    // 电池状态
    bat = ioI2C_readBattery();
    if (bat.chrg == 0) {
      blink = !blink;
      showBattery(bat.level ? (blink ? bat.level : bat.level - 1) : blink);
    } else if (batLast.level != bat.level) {
      showBattery(bat.level, bat.done == 0);
      batLast.raw = bat.raw;
    }

    // refreshActualIcons(); // 可选
  }

  delay(1);
}