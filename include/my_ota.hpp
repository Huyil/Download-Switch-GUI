#ifndef __MY_OTA_H_
#define __MY_OTA_H_

#include <arduino.h>

typedef struct{
    uint16_t post;
    String name;
    String passwd;
}MY_OTA_CFG;

typedef struct{
    void (*init)(void);
    void (*loop)(void);
}MY_OTA_OPS;

typedef struct{
    MY_OTA_CFG* cfg;
    MY_OTA_OPS* ops;
}MY_OTA;

extern MY_OTA my_ota;

#endif // !__MY_OTA_H_
