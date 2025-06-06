#ifndef __MY_WIFI_H__
#define __MY_WIFI_H__

#include <Arduino.h>
#include <WiFi.h>

typedef struct{
    String name;
    String passwd;
    String ip;
}MY_WIFI_CFG;

typedef struct{
    bool (*isconnected)(void);
    void (*connect)(void);
    void (*setSSID)(String ssid, String passwd);
}MY_WIFI_OPS;

typedef struct{
    MY_WIFI_CFG* cfg;
    MY_WIFI_OPS* ops;
}MY_WIFI;

extern MY_WIFI my_wifi;




#endif //__MY_WIFI_H__
