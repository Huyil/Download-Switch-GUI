#include "one_gui.h"

void SettingCB(void* arg)
{

}

void downLoadCB(void* arg)
{

}

void wifiCB(void* arg)
{

}

void BatteryCB(void* arg)
{

}

void arrowCB(void* arg)
{

}

//Init GUIElement
uint8_t GUIElement::nextId = 0;
GUIElement* GUIElement::head = nullptr;

// 设置图标
GUIElement settingIcon(160-15, 80-14, "/setting.bmp",SettingCB); 
// 下载图标
GUIElement downloadIcon(160-16, 37, "/download.bmp",downLoadCB); 
// 链接图标
GUIElement linkIcon(160-16, 24, "/link.bmp",0,RGB565_RED); 
// WiFi 图标
GUIElement wifiIcon(160-16, 10, "/wifi.bmp",wifiCB,RGB565_RED); 
// 电池符号
uint8_t batterPixels[120];
GUIElement batteryIcon(160-14, 0, "/battery.bmp", BatteryCB, RGB565_WHITE, 1.0, batterPixels, 120);

// 箭头图标
#define ArrowX 120  //60
#define ArrowY 50   //20
GUIElement arrowU(ArrowX + 18 * 0.5f, ArrowY + 0,         "/arrowU.bmp", 0, RGB565_WHITE, 0.5f);
GUIElement arrowR(ArrowX + 36 * 0.5f, ArrowY + 18 * 0.5f, "/arrowR.bmp", 0, RGB565_WHITE, 0.5f);
GUIElement arrowL(ArrowX + 0,         ArrowY + 18 * 0.5f, "/arrowL.bmp", 0, RGB565_WHITE, 0.5f);
GUIElement arrowD(ArrowX + 18 * 0.5f, ArrowY + 36 * 0.5f, "/arrowD.bmp", 0, RGB565_WHITE, 0.5f);
GUIElement arrowM(ArrowX + 20 * 0.5f, ArrowY + 20 * 0.5f, "/arrowM.bmp", 0, RGB565_WHITE, 0.5f);

// 设置图标
GUIElement arrowIcon(56, 0, "/arrow.bmp",arrowCB ,RGB565_PALERED,1.0); 

void showArrow(uint8_t key)
{
    arrowU.setColor((key & 0x01) ? RGB565_WHITE : RGB565(102, 204, 255));
    arrowR.setColor((key & 0x02) ? RGB565_WHITE : RGB565(102, 204, 255));
    arrowL.setColor((key & 0x04) ? RGB565_WHITE : RGB565(102, 204, 255));
    arrowD.setColor((key & 0x08) ? RGB565_WHITE : RGB565(102, 204, 255));
    arrowM.setColor((key & 0x10) ? RGB565_WHITE : RGB565(102, 204, 255));

    arrowU.draw();
    arrowR.draw();
    arrowL.draw();
    arrowD.draw();
    arrowM.draw();
}


void showWifiState(bool sw)
{
    wifiIcon.setColor(sw ? RGB565_WHITE : RGB565_RED);
    wifiIcon.draw();
}



void showLinkState(bool sw)
{
    linkIcon.setColor(sw ? RGB565_WHITE : RGB565_RED);
    linkIcon.draw();
}

// 电池图标保留函数处理逻辑（图像内容会被动态修改）
// cells定义，和格子大小
const struct { uint8_t x0, y0, x1, y1; } cells[4] = {
    {2, 2, 3, 7}, 
    {4, 2, 5, 7}, 
    {6, 2, 7, 7}, 
    {8, 2, 9, 7}
};

uint8_t levelPixels[2 * 6]; // 假设最大宽2，高6，存储单个格子原始像素

// 把 levelPixels 复制回 batteryIcon.info.buffer 对应的格子区域
void restoreCellInBuffer(GUIElement &batteryIcon, uint8_t cellIndex) {
    if (!batteryIcon.info.buffer) return;
    uint8_t w = cells[cellIndex].x1 - cells[cellIndex].x0 + 1;
    uint8_t h = cells[cellIndex].y1 - cells[cellIndex].y0 + 1;

    for (uint8_t y = 0; y < h; ++y) {
        for (uint8_t x = 0; x < w; ++x) {
            batteryIcon.info.buffer[(cells[cellIndex].y0 + y) * batteryIcon.info.width + (cells[cellIndex].x0 + x)]
                = levelPixels[y * w + x];
        }
    }
}

// 把对应格子区域填充为0（清空格子）
void clearCellInBuffer(GUIElement &batteryIcon, uint8_t cellIndex) {
    if (!batteryIcon.info.buffer) return;
    uint8_t w = cells[cellIndex].x1 - cells[cellIndex].x0 + 1;
    uint8_t h = cells[cellIndex].y1 - cells[cellIndex].y0 + 1;

    for (uint8_t y = 0; y < h; ++y) {
        for (uint8_t x = 0; x < w; ++x) {
            batteryIcon.info.buffer[(cells[cellIndex].y0 + y) * batteryIcon.info.width + (cells[cellIndex].x0 + x)] = 0;
        }
    }
}

// 主显示函数示例，level为当前电量格子数，满4格
void showBattery(uint8_t level, bool full) {
    if (!batteryIcon.isLoad) {
        // 第一次加载BMP图，生成buffer缓存（满电池）
        batteryIcon.isShow = false;  // 不显示这次，只缓存
        batteryIcon.draw();

        // 拷贝第一个格子的原始满电像素
        for (uint8_t y = 0; y < 6; ++y) {
            for (uint8_t x = 0; x < 2; ++x) {
                levelPixels[y * 2 + x] = batteryIcon.info.buffer[(cells[0].y0 + y) * batteryIcon.info.width + (cells[0].x0 + x)];
            }
        }
    }

    // 根据level修改buffer中对应格子
    for (uint8_t i = 0; i < 4; ++i) {
        if (i < level) {
            restoreCellInBuffer(batteryIcon, i);  // 恢复满格像素
        } else {
            clearCellInBuffer(batteryIcon, i);    // 清空空格子
        }
    }

    // 绘制整张电池图（使用buffer）
    batteryIcon.isShow = true;
    batteryIcon.isRefresh = true;
    batteryIcon.color = full ? RGB565_GREEN : RGB565_WHITE;
    batteryIcon.draw();
}


void gui_init(void)
{
    gui.lcd_init();
    
    showArrow(0x1F);
    showBattery(0, 0);
    showLinkState(0);
    showWifiState(0);
    downloadIcon.draw();
    settingIcon.draw();
    arrowIcon.draw();
}