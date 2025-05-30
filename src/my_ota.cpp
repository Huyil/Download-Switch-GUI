#include "../include/my_ota.hpp"
#include "../include/my_wifi.hpp"

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
static void InitOTA(void){

  if(my_ota.cfg->post != 0)
    ArduinoOTA.setPort(my_ota.cfg->post); // Port defaults to 3232

  if(my_ota.cfg->name != NULL)
    ArduinoOTA.setHostname(my_ota.cfg->name.c_str()); // Hostname defaults to esp3232-[MAC]

  if(my_ota.cfg->passwd != NULL)
    ArduinoOTA.setPassword(my_ota.cfg->passwd.c_str()); // No authentication by default

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
      // motor.ops->keepLow();
      // digitalWrite(10,1);
    })
    .onEnd([]() {
      pinMode(2, OUTPUT);
      digitalWrite(2,0);      //释放IO2
      Serial.println("\nEnd");
      abort();
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  if(my_ota.cfg->name == NULL)
  {

  }
}

static void loopOTA(void){
  ArduinoOTA.handle();
}

static MY_OTA_CFG cfg ={
  .post = 3232,       //默认端口
  .name = "ESP32C3-SwitchDown",
  .passwd = "12345678",
};

static MY_OTA_OPS ops = {
    .init = InitOTA,
    .loop = loopOTA,
};

MY_OTA my_ota = {
  .cfg = &cfg,
  .ops = &ops,
};
