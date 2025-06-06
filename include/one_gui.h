#ifndef __one_gUi_H__
#define __one_gUi_H__

#include "one_gfx.h"

void gui_init(void);
void showArrow(uint8_t key);
void showBattery(uint8_t level,bool full = 0);
void showLinkState(bool sw);
void showWifiState(bool sw);

#endif //__one_gUi_H__


