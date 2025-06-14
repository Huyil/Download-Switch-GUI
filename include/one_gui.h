#ifndef __one_gUi_H__
#define __one_gUi_H__

#include "one_gfx.h"


struct IconEntry {
  uint8_t index;        // 图标序号
  const char* label;    // 显示的文字
  uint16_t color;       // 文件名
};
struct IconChannel {
    int x, y;           // 坐标
    uint8_t mode;       // 引用 signalInfo 中的模式编号
    bool isLock;          // 是否为锁定图标（true 用 signalLock，false 用 signalIcon）
};

void gui_init(void);
void showArrow(uint8_t key);
void showBattery(uint8_t level,bool full = 0);
void showLinkState(bool sw);
void showWifiState(bool sw);
void refreshActualIcons();
#endif //__one_gUi_H__


