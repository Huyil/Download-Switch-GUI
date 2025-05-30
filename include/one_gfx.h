#ifndef ONE_GFX_H
#define ONE_GFX_H

#include "Arduino.h"
#include <Arduino_GFX_Library.h>
#include <LittleFS.h>

void lcd_init();
void showArrow(uint8_t key);
void showBattery(uint8_t level);
void showLinkState(bool sw);

#endif
