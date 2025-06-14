#include "one_gui.h"
#include "one_i2cIO.h"

void saveChannelConfig();
void loadChannelConfig();
uint8_t SettingCB(void* arg);
uint8_t downLoadCB(void* arg);
uint8_t wifiCB(void* arg);
uint8_t BatteryCB(void* arg);
uint8_t arrowCB(void* arg);

//Init GUIElement
uint8_t GUIElement::nextId = 0;
GUIElement* GUIElement::head = nullptr;

// 设置图标
GUIElement settingIcon(160-15, 80-14, "/setting.bmp",SettingCB); 
// 下载图标
GUIElement downloadIcon(160-16, 37, "/download.bmp",downLoadCB); 
// 链接图标
GUIElement linkIcon(160-16, 12, "/link.bmp",0,RGB565_RED); 
// WiFi 图标
GUIElement wifiIcon(160-16, 23, "/wifi.bmp",wifiCB,RGB565_RED); 
// 电池符号
uint8_t batterPixels[120];
GUIElement batteryIcon(160-14, 0, "/battery.bmp", BatteryCB, RGB565_WHITE, 1.0, batterPixels, sizeof(batterPixels));

// 箭头图标
#define ArrowX 120  //60
#define ArrowY 50   //20
GUIElement arrowU(ArrowX + 18 * 0.5f, ArrowY + 0,         "/arrowU.bmp", 0, RGB565_WHITE, 0.5f);
GUIElement arrowR(ArrowX + 36 * 0.5f, ArrowY + 18 * 0.5f, "/arrowR.bmp", 0, RGB565_WHITE, 0.5f);
GUIElement arrowL(ArrowX + 0,         ArrowY + 18 * 0.5f, "/arrowL.bmp", 0, RGB565_WHITE, 0.5f);
GUIElement arrowD(ArrowX + 18 * 0.5f, ArrowY + 36 * 0.5f, "/arrowD.bmp", 0, RGB565_WHITE, 0.5f);
GUIElement arrowM(ArrowX + 20 * 0.5f, ArrowY + 20 * 0.5f, "/arrowM.bmp", 0, RGB565_WHITE, 0.5f);

// 设置图标
GUIElement arrowIcon(52, 5, "/arrow.bmp",arrowCB ,RGB565_BLACK,1.0); 

// 提前准备缓存像素数组
uint8_t signalPixels[18 * 18];
uint8_t signalLockPixels[18 * 18];

// 创建 GUIElement 实例（只创建一次）
GUIElement signalIcon(0, 0, "/signal.bmp", nullptr, RGB565_WHITE, 1.0, signalPixels, sizeof(signalPixels));
GUIElement signalLock(0, 0, "/signalLock.bmp", nullptr, RGB565_WHITE, 1.0, signalLockPixels, sizeof(signalLockPixels));

enum CHxMode {
    MODE_NC = 0,
    MODE_GND,
    MODE_SCK,
    MODE_SDA,
    MODE_VCC,
    MODE_num
};

// 图标信息数组
IconEntry signalInfo[5] = {
    {MODE_NC,  "NC ", RGB565(80, 80, 80)},
    {MODE_GND, "GND", RGB565(0, 0, 255)},
    {MODE_SCK, "SCK", RGB565(0, 255, 0)},
    {MODE_SDA, "SDA", RGB565(255, 0, 255)},
    {MODE_VCC, "VCC", RGB565(255, 0, 0)},
};

IconChannel channels[4] = {
    {0, 0,  0, 0},
    {0, 20, 0, 0},
    {0, 40, 0, 0},
    {0, 60, 0, 0},
};
void refreshIcons();

uint8_t SettingCB(void* arg)
{
    return 0;
}

uint8_t downLoadCB(void* arg)
{
    return 0;
}

uint8_t wifiCB(void* arg)
{
    return 0;
}

uint8_t BatteryCB(void* arg)
{
    return 0;
}

int8_t selectedChannel = 0;  // 当前通道编号（0~3）
uint8_t arrowCB(void* arg) {
    uint8_t keyValue = (uint8_t)(uintptr_t)arg;

    bool isFrist = !!(keyValue & 0x80);
    uint8_t key = keyValue & 0x3F;

    GUIElement* elem = &arrowIcon; // 当前图标

    if (isFrist) {
        if (!(keyValue & 0x01)) { // 向上
            selectedChannel = 3;
        } else if (!(keyValue & 0x08)) {
            selectedChannel = 0;
        }
        // 合法通道，移动箭头
        elem->y = channels[selectedChannel].y+5;
        elem->isRefresh = true;
        elem->isSelected = true;
        elem->draw();
        return 0; // 切换
    } else {
        if ((key & 0x04) == 0) {
            // 向左：切换当前通道是否为锁定图标
            channels[selectedChannel].isLock = !channels[selectedChannel].isLock;
            refreshIcons();

            // 判断是否包含 GND, SCK, SDA（VCC可选）
            bool hasGND = false, hasSCK = false, hasSDA = false;

            for (int i = 0; i < 4; i++) {
                if (channels[i].isLock) {
                    switch (channels[i].mode) {
                        case MODE_GND: hasGND = true; break;
                        case MODE_SCK: hasSCK = true; break;
                        case MODE_SDA: hasSDA = true; break;
                    }
                }
            }

            if (hasGND && hasSCK && hasSDA) {
                saveChannelConfig();
                Serial.println("Saved channel config");
                gui.bmpStringWithColor(0, 80, F("Saved config"), RGB565_GREEN);
            }

            return 1; // 阻止切换
        }else if ((key & 0x02) == 0 )  {
            if(channels[selectedChannel].isLock==0)
            {    // 向右：切换模式
                IconChannel& ch = channels[selectedChannel];
                CHxMode nextMode = static_cast<CHxMode>((ch.mode + 1) % MODE_num);
                
                // 查找是否有其他 isLock 且使用了 SCK/SDA
                bool sckUsed = false;
                bool sdaUsed = false;
                for (int i = 0; i < 4; ++i) {
                    if (i != selectedChannel && channels[i].isLock) {
                        if (channels[i].mode == MODE_SCK) sckUsed = true;
                        if (channels[i].mode == MODE_SDA) sdaUsed = true;
                    }
                }

                // 尝试跳过被占用的模式
                for (int i = 0; i < MODE_num; ++i) {
                    if ((nextMode == MODE_SCK && sckUsed) || (nextMode == MODE_SDA && sdaUsed)) {
                        nextMode = static_cast<CHxMode>((nextMode + 1) % MODE_num);
                    } else {
                        break; // 可用模式
                    }
                }

                ch.mode = nextMode;
                refreshIcons();
            }
        } else if ((key & 0x01) == 0) { // 向上
            selectedChannel--;
        } else if ((key & 0x08) == 0) { // 向下
            selectedChannel++;
        }

        // 判断是否越界
        elem->clear();
        if (selectedChannel < 0 || selectedChannel >= 4) {
            elem->isSelected = 0;
            return 0; // 允许切换
        }

        // 合法通道，移动箭头
        elem->y = channels[selectedChannel].y+5;
        elem->isRefresh = true;
        elem->isSelected = true;
        elem->draw();
        return 1; // 阻止切换
    }
    return 1;// 阻止切换
}

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
    if (!LittleFS.begin())
    {
        Serial.println(F("ERROR: File System Mount Failed!"));
        gui.bmpStringWithColor(0,0,F("ERROR: File System Mount Failed!"),RGB565_WHITE);
    }else{
        loadChannelConfig();
    }
    
    // showArrow(0x1F);
    showBattery(0, 0);
    showLinkState(0);
    showWifiState(0);
    downloadIcon.draw();
    settingIcon.draw();

    arrowIcon.load();
    signalLock.load();
    signalIcon.load();
    refreshIcons();
}

void refreshIcons() {
    for (int i = 0; i < 4; i++) {
        const IconChannel& ch = channels[i];
        const IconEntry& entry = signalInfo[ch.mode];  // 取对应的模式信息

        
        GUIElement* target = ch.isLock ? &signalLock : &signalIcon;
        target->setPosition(ch.x, ch.y);
        target->setColor(entry.color);
        target->isRefresh = true;
        target->draw();

        if(ch.isLock)
            ioI2C_setChannelMode(i,ch.mode);
        else ioI2C_setChannelMode(i,0);
        gui.bmpStringWithColor(ch.x + 22, ch.y, String(entry.label), RGB565_WHITE);
    }
}


void refreshIOStatus(IORaw io) {
    // IORaw io = ioI2C_readIO();

    // 坐标起点与间隔
    const int x1 = 50, x2 = 60;
    const int yStart = 0;
    const int yStep = 10;

    // 第一列：显示 P1~P4 和 N1~N4（共8位）
    bool pnBits[8] = {
        io.io.P1, io.io.N1,
        io.io.P2, io.io.N2,
        io.io.P3, io.io.N3,
        io.io.P4, io.io.N4,
    };

    for (int i = 0; i < 8; i++) {
        int y = yStart + i * yStep;
        uint16_t color = pnBits[i] ? (i&0x01?RGB565_GREEN:RGB565_RED) : RGB565_DARKGREY;

        gui.gfx->fillRect(x1, y, 8, 8, color);  // 小方块大小 8x8
    }

    // 第二列：显示 CLK 和 DIO 控制（共6位）
    bool ioBits[6] = {
        io.io.CLK_A, io.io.CLK_B, io.io.CLK_EN,
        io.io.DIO_A, io.io.DIO_B, io.io.DIO_EN,
    };

    for (int i = 0; i < 6; i++) {
        int y = yStart + i * yStep;
        uint16_t color = ioBits[i] ? RGB565_RED : RGB565_DARKGREY;
        gui.gfx->fillRect(x2, y, 8, 8, color);
    }

    // // 显示 ioset 为十六进制
    // char buf1[12];
    // sprintf(buf1, "0x%08lX", io.ioset);  // 8位十六进制大写（32位）
    // gui.bmpStringWithColor(70, 0, buf1, RGB565_YELLOW);

    // // 显示 ch1-ch4 拼接后的值（高位ch1）
    // uint16_t packed = (io.ch4 << 9) | (io.ch3 << 6) | (io.ch2 << 3) | io.ch1;
    // char buf2[10];
    // sprintf(buf2, "0x%04X", packed);
    // gui.bmpStringWithColor(70, 20, buf2, RGB565_YELLOW);

    // // 显示 ch1-ch4 拼接后的值（高位ch1）
    // uint16_t packed2 = (channels[3].mode << 9) | (channels[2].mode << 6) | (channels[1].mode << 3) | channels[0].mode;
    // char buf3[10];
    // sprintf(buf3, "0x%04X", packed2);
    // gui.bmpStringWithColor(70, 40, buf3, RGB565_BLUE);
}

void refreshActualIcons() {
    static uint32_t lastRealMode[4] = {0xFF, 0xFF, 0xFF, 0xFF}; // 初始设为无效值，强制第一次刷新

    IORaw io = ioI2C_readIO();
    
    //refreshIOStatus(io);
    for (int i = 0; i < 4; i++) {
        const IconChannel& ch = channels[i];
        uint8_t realMode = io.ch[i].mode;

        // 仅当读取值变化，且与设置值不一致时刷新
        if (realMode != lastRealMode[i]) {
            lastRealMode[i] = realMode; // 更新缓存

            const IconEntry& realEntry = signalInfo[realMode];

            GUIElement* actual = &signalIcon; // 或使用 signalActualIcon
            actual->setPosition(ch.x, ch.y);
            actual->setColor(realEntry.color);
            actual->isRefresh = true;
            actual->draw();

            //gui.bmpStringWithColor(22, ch.y, String(realEntry.label), RGB565_YELLOW);
        }
    }
}

//LittleFS

void loadChannelConfig() {
    File f = LittleFS.open("/channels.cfg", "r");
    if (!f || f.size() < 4) {
        Serial.println("No valid config");
        return;
    }

    for (int i = 0; i < 4; i++) {
        int m = f.read();
        if (m > 0 && m < MODE_num)
        {
            channels[i].mode = (uint8_t)m;
            channels[i].isLock = 1;
        }
    }

    f.close();
}

void saveChannelConfig() {
    File f = LittleFS.open("/channels.cfg", "w");
    if (!f) return;

    for (int i = 0; i < 4; i++) {
        f.write(channels[i].mode);
    }

    f.close();
}

