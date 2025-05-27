#include "one_gfx.h"
#include <Arduino_GFX_Library.h>
// #include <font/FreeMono9pt7b.h>   // 引入字体头文件
#include <LittleFS.h>
#include "BmpClass.h"

static BmpClass bmpClass;

#define GFX_BL 10
Arduino_DataBus *bus = new Arduino_ESP32SPI(2 /* DC */, 4 /* CS */, 1 /* SCK */,0 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_ST7735(bus, 3 /* RST */, 1 /* rotation */, false /* IPS */,80 /* width */, 160 /* height */,24 /* col offset 1 */, 0 /* row offset 1 */,24 /* col offset 2 */, 0 /* row offset 2 */);

#define FONT_WIDTH 8
#define FONT_HEIGHT 16
#define CHAR_BYTES (FONT_WIDTH*FONT_HEIGHT)  // 每个字符的字节数 (16x8 位 = 16 字节)
#define ASCII_OFFSET 0x20
#define FONT_FILE "/font16x8.bin"

#define ICON_WIDTH 18
#define ICON_HEIGHT 18
#define ICON_COUNT 4

struct IconEntry {
  uint8_t index;      // 图标序号
  const char* name;   // 文件名
  const char* label;    // 显示的文字
  File file;          // LittleFS 返回的 File 对象
};

// 图标信息数组
IconEntry icons[ICON_COUNT] = {
  {0, "/LEDR.bmp", "VCC", File()},
  {1, "/LEDG.bmp", "SCK", File()},
  {2, "/LEDB.bmp", "GND", File()},
  {3, "/LEDP.bmp", "SDA", File()},
};
void initIcons(void);
void refreshIcons(void);


// pixel drawing callback
static void bmpDrawCallback(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h)
{
  // Serial.printf("Draw pos = %d, %d. size = %d x %d\n", x, y, w, h);
  gfx->draw16bitRGBBitmap(x, y, bitmap, w, h);
}

static void bmpDrawGray8WithColor(int16_t x, int16_t y, const char* path, uint16_t color)
{
  File f = LittleFS.open(path, "r");
  if (!f) {
    Serial.printf("Failed to open BMP file: %s\n", path);
    return;
  }

  uint8_t header[54];
  if (f.read(header, 54) != 54 || header[0] != 'B' || header[1] != 'M') {
    Serial.println("Invalid BMP file.");
    f.close();
    return;
  }

  uint16_t width  = header[18] | (header[19] << 8);
  uint16_t height = header[22] | (header[23] << 8);
  uint16_t bpp    = header[28];
  uint32_t offset = header[10] | (header[11] << 8) | (header[12] << 16) | (header[13] << 24);

  if (bpp != 8) {
    Serial.printf("Unsupported BMP format (must be 8bpp): %d bpp\n", bpp);
    f.close();
    return;
  }

  uint32_t rowSize = (width + 3) & ~3; // 行数据4字节对齐

  uint8_t* grayBuf = (uint8_t*)malloc(width * height);
  if (!grayBuf) {
    Serial.println("malloc failed for grayBuf.");
    f.close();
    return;
  }

  uint8_t* rowBuf = (uint8_t*)malloc(rowSize);
  if (!rowBuf) {
    Serial.println("malloc failed for rowBuf.");
    free(grayBuf);
    f.close();
    return;
  }

  // BMP 行是倒序存储，从底向上
  for (uint16_t yRow = 0; yRow < height; yRow++) {
    f.seek(offset + (height - 1 - yRow) * rowSize);
    f.read(rowBuf, rowSize);
    memcpy(&grayBuf[yRow * width], rowBuf, width);
  }

  // 显示图像
  gfx->drawGrayWithColorBitmap(x, y, grayBuf, color, width, height);

  free(rowBuf);
  free(grayBuf);
  f.close();
}

static void bmpStringWithColor(uint8_t Stax, uint8_t Stay, const String& str)
{
  File fontFile = LittleFS.open(FONT_FILE, "r");
  if (!fontFile) {
    Serial.println("Failed to open font file");
    return;
  }

  uint8_t fontBuf[CHAR_BYTES];

  for (size_t i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (c < ASCII_OFFSET || c > '~') {
      c = ' ';  // 替换为空格
    }

    size_t offset = (c - ASCII_OFFSET) * CHAR_BYTES;
    fontFile.seek(offset, SeekSet);
    fontFile.read(fontBuf, CHAR_BYTES);

    gfx->drawGrayWithColorBitmap(Stax, Stay, fontBuf, RGB565_WHITE, FONT_WIDTH, FONT_HEIGHT);
    Stax += 9; // 每个字符之间的间隔
  }

  fontFile.close(); // 关闭文件
}

void lcd_init(){
  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(RGB565_BLACK);
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
  
  // gfx->setFont(&FreeMono9pt7b);    // 设置字体
  // gfx->setTextColor(WHITE);
  // gfx->setTextSize(1); // 或 gfx->setFont(...); 可设置字体大小

  if (!LittleFS.begin())
  {
    Serial.println(F("ERROR: File System Mount Failed!"));
    gfx->println(F("ERROR: File System Mount Failed!"));
  }else{
    initIcons();
    refreshIcons();
    #define ArrowX 100
    #define ArrowY 10
    bmpDrawGray8WithColor(ArrowX+20, ArrowY,    "/arrowU.bmp", RGB565_WHITE);
    bmpDrawGray8WithColor(ArrowX+20, ArrowY+40, "/arrowD.bmp", RGB565_WHITE);
    bmpDrawGray8WithColor(ArrowX,    ArrowY+20, "/arrowL.bmp", RGB565_WHITE);
    bmpDrawGray8WithColor(ArrowX+40, ArrowY+20, "/arrowR.bmp", RGB565_WHITE);
    bmpDrawGray8WithColor(ArrowX+22, ArrowY+22, "/arrowM.bmp", RGB565(102, 204, 255));
  }
}

void initIcons(void) {
  for (int i = 0; i < ICON_COUNT; i++) {
    icons[i].file = LittleFS.open(icons[i].name, "r");

    if (!icons[i].file) {
      Serial.printf("Failed to open: %s\n", icons[i].name);
    }
  }
}

void refreshIcons() {
  for (int i = 0; i < ICON_COUNT; i++) {
    int x = 0;
    int y = i * 20;

    if (icons[i].file) {
      icons[i].file.seek(0);  // 重置文件指针
      bmpClass.draw(
        &icons[i].file,
        bmpDrawCallback,
        false,          // 不使用大端
        x, y,           // 显示坐标
        ICON_WIDTH, ICON_HEIGHT          // 限制区域范围
      );
    }
    
    // 绘制文字（在图标右边 22 像素开始，保持对齐）
    // gfx->setCursor(22, y + 14);  // +4 为垂直居中调节
    // gfx->print(icons[i].label);
    bmpStringWithColor(22, y, String(icons[i].label));
  }
}