#ifndef ONE_GFX_H
#define ONE_GFX_H

#include "Arduino.h"
#include <Arduino_GFX_Library.h>
#include <LittleFS.h>


// BMP 灰度图结构（用于 loadBMPGray8）
typedef struct {
    uint8_t *pixels = nullptr;
    uint16_t width = 0;
    uint16_t height = 0;
} BMPGray8;

/// 图像资源信息
/**
 * @brief 图形用户界面元素信息结构体
 * 
 * 包含图像资源的路径、缓冲区、尺寸和缩放比例等信息
 * 
 * @param path 图像资源路径或名称
 * @param buffer 图像数据缓冲区指针
 * @param bufferSize 缓冲区大小
 * @param width 图像宽度(像素)
 * @param height 图像高度(像素)
 * @param scale 图像缩放比例
 */
struct GUIElementInfo {
    const char *path;
    uint8_t *buffer;
    size_t bufferSize;
    float scale;
    uint8_t width;
    uint8_t height;
    void (*onInsert)(void*);

    GUIElementInfo(const char* p, float s = 1.0f, uint8_t* buf = nullptr, size_t size = 0,
                   void(*insertFunc)(void*) = nullptr, uint8_t w = 0, uint8_t h = 0)
        : path(p), buffer(buf), bufferSize(size), width(w), height(h), scale(s), onInsert(insertFunc){}

    void setScale(float s) {
        if (s > 0) scale = s;
    }
};

struct GUIElement {
    GUIElementInfo info;
    int16_t x, y;
    uint16_t color;
    bool isLoad;
    bool isShow;
    bool isRefresh;
    bool isSelected;
    
    uint8_t id;         // 唯一 ID
    // 链表指针
    GUIElement* next;
    GUIElement* prev;    // 新增双向链表指针
    
    // 静态成员：记录已分配ID数量和链表头
    static uint8_t nextId;
    static GUIElement* head;

    // 主构造函数
    GUIElement(int16_t _x, int16_t _y, 
               const char* path = "/error.bmp", 
               void (*_onInsert)(void*) = nullptr,
               uint16_t _color = RGB565_WHITE,
               float scale = 1.0f,
               uint8_t* buffer = nullptr,
               size_t bufferSize = 0)
        : info(path, scale, buffer, bufferSize, _onInsert),
          x(_x), y(_y),
          color(_color),
          isShow(true),
          isRefresh(true),
          isLoad(false),
          id(nextId++),  // 分配新ID
          next(nullptr)//, prev(nullptr)
        {
            if(!_onInsert) return;
            if (!head) {
                head = this;
                next = this;
                prev = this;
            } else {
                // 插入头部，维护双向循环链表
                GUIElement* tail = head->prev;
                tail->next = this;
                this->prev = tail;
                this->next = head;
                head->prev = this;
                head = this;
            }
        }

    // 根据ID查找元素（静态方法）
    static GUIElement* findById(uint8_t targetId) {
        if (!head) return nullptr; // 空链表直接返回

        GUIElement* current = head;
        do {
            if (current->id == targetId) {
                return current;
            }
            current = current->next;
        } while (current != head);

        return nullptr; // 遍历了一圈没找到
    }
          
    // 设置位置
    void setPosition(int16_t newX, int16_t newY) {
        if (x != newX || y != newY) {
            x = newX;
            y = newY;
            isRefresh = true;  // 位置变了需要刷新
        }
    }

    // 设置颜色
    void setColor(uint16_t newColor) {
        if (color != newColor) {
            color = newColor;
            isRefresh = true;  // 颜色变了需要刷新
        }
    }

    // 设置缩放比例
    void setScale(float newScale) {
        info.setScale(newScale);
        isRefresh = true; // 缩放变了需要刷新
    }

    
    // 使用当前成员绘制
    void draw(void);
};

// 图像管理类
class GUIManager {
private:
    // 屏幕驱动
    Arduino_GFX* gfx;
public:
    GUIElement* currentSelected; // 当前选中的元素指针
    GUIManager(Arduino_GFX* gfxInstance): gfx(gfxInstance){};
    // 处理按键输入并更新选中元素
    uint8_t handleKeyInput(uint8_t keyValue) {
        if (!GUIElement::head) return 0; // 空链表直接返回

        // 初始化当前选中（如果为空）
        if (!currentSelected) {
            currentSelected = GUIElement::head;
            currentSelected->isRefresh = true;
            return 0;
        }

        GUIElement* prevSelected = currentSelected;

        // 按键处理逻辑
        if ((keyValue & 0x01) == 0) { // 上键
            currentSelected = currentSelected->prev;
        } else if ((keyValue & 0x08) == 0) { // 下键
            currentSelected = currentSelected->next;
        }

        // 更新刷新状态
        if (prevSelected != currentSelected) {
            prevSelected->isRefresh = true;
            currentSelected->isRefresh = true;

            prevSelected->isSelected = false;
            currentSelected->isSelected = true;
            return 1;
        }
        return 0;
    }

    // 绘图
    void lcd_init();
    void bmpDrawGray8WithColor(GUIElement* picObj);
    void bmpDrawGray8WithColor(int16_t x, int16_t y, const char* path, uint16_t color, float scale = 1.0f);
    void bmpStringWithColor(uint8_t Stax, uint8_t Stay, const String str,uint16_t color);
    void drawSelectionBox(GUIElement* elem);
};

extern GUIManager gui;

#endif
