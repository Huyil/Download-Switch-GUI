#include <Arduino.h>
#include "one_gfx.h"


void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.setDebugOutput(true);
  Serial.println("Starting setup...");
  lcd_init();
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(5000);
}
