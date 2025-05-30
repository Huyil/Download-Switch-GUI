#include "one_gfx.h"
// #include <font/FreeMono9pt7b.h>   // 引入字体头文件
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


struct IconEntry {
  uint8_t index;        // 图标序号
  const char* label;    // 显示的文字
  uint16_t color;       // 文件名
  bool lock;
};

#define ICON_WIDTH 18
#define ICON_HEIGHT 18
#define ICON_COUNT 4
struct IconSignal
{
  struct IconEntry Info[ICON_COUNT];
  String bmpName;
  String bmpLockName;
  uint8_t bmp[ICON_WIDTH*ICON_HEIGHT];
  uint8_t bmpLock[ICON_WIDTH*ICON_HEIGHT];
  
  uint8_t width, height;
};

// 图标信息数组
struct IconSignal signalIO = {
  .Info={
    {0, "VCC", RGB565(255,0,0)},
    {1, "SCK", RGB565(0,255,0)},
    {2, "GND", RGB565(0,0,255)},
    {3, "SDA", RGB565(255,0,255)},
  },
  .bmpName = "/signal.bmp",
  .bmpLockName = "/signalLock.bmp",
  .width = ICON_WIDTH,
  .height = ICON_HEIGHT
};

struct BMPGray8 {
  uint8_t* pixels = nullptr;
  uint16_t width = 0;
  uint16_t height = 0;
};

void initIcons(void);
void refreshIcons(void);


// pixel drawing callback
static void bmpDrawCallback(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h)
{
  // Serial.printf("Draw pos = %d, %d. size = %d x %d\n", x, y, w, h);
  gfx->draw16bitRGBBitmap(x, y, bitmap, w, h);
}

bool loadBMPGray8(const char* path, BMPGray8& outBmp, float scale = 1.0f, size_t bufSize = 0)
{
  File f = LittleFS.open(path, "r");
  if (!f) {
    Serial.printf("Failed to open BMP file: %s\n", path);
    return false;
  }

  uint8_t header[54];
  if (f.read(header, 54) != 54 || header[0] != 'B' || header[1] != 'M') {
    Serial.println("Invalid BMP file.");
    f.close();
    return false;
  }

  uint16_t width  = header[18] | (header[19] << 8);
  uint16_t height = header[22] | (header[23] << 8);
  uint16_t bpp    = header[28];
  uint32_t offset = header[10] | (header[11] << 8) | (header[12] << 16) | (header[13] << 24);

  if (bpp != 8) {
    Serial.printf("Unsupported BMP format (must be 8bpp): %d bpp\n", bpp);
    f.close();
    return false;
  }

  uint32_t rowSize = (width + 3) & ~3;
  size_t rawPixelCount = (size_t)width * height;

  uint8_t* rawBuf = (uint8_t*)malloc(rawPixelCount);
  if (!rawBuf) {
    Serial.println("malloc failed for rawBuf.");
    f.close();
    return false;
  }

  uint8_t* rowBuf = (uint8_t*)malloc(rowSize);
  if (!rowBuf) {
    Serial.println("malloc failed for rowBuf.");
    free(rawBuf);
    f.close();
    return false;
  }

  for (uint16_t yRow = 0; yRow < height; yRow++) {
    f.seek(offset + (height - 1 - yRow) * rowSize);
    f.read(rowBuf, rowSize);
    memcpy(&rawBuf[yRow * width], rowBuf, width);
  }

  free(rowBuf);
  f.close();

  if (scale == 1.0f) {
    // 直接使用原图数据
    if (outBmp.pixels != nullptr && bufSize >= rawPixelCount) {
      memcpy(outBmp.pixels, rawBuf, rawPixelCount);
      free(rawBuf);  // 原始数据复制完就可以释放
    } else {
      outBmp.pixels = rawBuf; // 直接使用原始内存
    }
    outBmp.width = width;
    outBmp.height = height;
    return true;
  }

  // 否则进行缩放
  uint16_t dstW = max((uint16_t)1, (uint16_t)(width * scale));
  uint16_t dstH = max((uint16_t)1, (uint16_t)(height * scale));
  size_t dstSize = (size_t)dstW * dstH;

  uint8_t* outBuf = outBmp.pixels;
  bool useExternal = (outBuf != nullptr && bufSize >= dstSize);

  if (!useExternal) {
    outBuf = (uint8_t*)malloc(dstSize);
    if (!outBuf) {
      Serial.println("malloc failed for scaled buffer.");
      free(rawBuf);
      return false;
    }
  }

  // 最近邻缩放
  for (uint16_t y = 0; y < dstH; ++y) {
    uint16_t srcY = min((uint16_t)(y / scale), (uint16_t)(height - 1));
    for (uint16_t x = 0; x < dstW; ++x) {
      uint16_t srcX = min((uint16_t)(x / scale), (uint16_t)(width - 1));
      outBuf[y * dstW + x] = rawBuf[srcY * width + srcX];
    }
  }

  free(rawBuf);

  outBmp.pixels = outBuf;
  outBmp.width = dstW;
  outBmp.height = dstH;
  return true;
}



static void bmpDrawGray8WithColor(int16_t x, int16_t y, const char* path, uint16_t color,float scale=1.0f)
{
  BMPGray8 bmp;
  if (!loadBMPGray8(path, bmp, scale)) {
    Serial.println("Failed to load BMP.");
    return;
  }

  gfx->drawGrayWithColorBitmap(x, y, bmp.pixels, color, bmp.width, bmp.height);
  free(bmp.pixels); // 注意释放内存
}

static void bmpStringWithColor(uint8_t Stax, uint8_t Stay, const String str)
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
    showArrow(0x0F);
    showBattery(0);
    showLinkState(0);
    
    bmpDrawGray8WithColor(160-16,9,"/wifi.bmp", RGB565_WHITE);
    bmpDrawGray8WithColor(160-13,80-14, "/setting.bmp", RGB565_WHITE);
  }
}

void initIcons(void) {
  BMPGray8 _bmp;
  _bmp.pixels = signalIO.bmp;
  loadBMPGray8(signalIO.bmpName.c_str(),_bmp, 1.0, sizeof(signalIO.bmp));
  
  _bmp.pixels = signalIO.bmpLock;
  loadBMPGray8(signalIO.bmpLockName.c_str(),_bmp, 1.0, sizeof(signalIO.bmpLock));
}

void refreshIcons() {
  for (int i = 0; i < ICON_COUNT; i++) {
    int x = 0;
    int y = i * 20;
    gfx->drawGrayWithColorBitmap(x, y, 
      signalIO.Info[i].lock ? signalIO.bmpLock : signalIO.bmp,
      signalIO.Info[i].color, signalIO.width, signalIO.height);

    bmpStringWithColor(22, y, String(signalIO.Info[i].label));
  }
}

#define ArrowX 60
#define ArrowY 20
void showArrow(uint8_t key)
{
    bmpDrawGray8WithColor(ArrowX+18, ArrowY,    "/arrowU.bmp", (key&0x01)?RGB565_WHITE:RGB565(102, 204, 255));
    bmpDrawGray8WithColor(ArrowX+36, ArrowY+18, "/arrowR.bmp", (key&0x02)?RGB565_WHITE:RGB565(102, 204, 255));
    bmpDrawGray8WithColor(ArrowX,    ArrowY+18, "/arrowL.bmp", (key&0x04)?RGB565_WHITE:RGB565(102, 204, 255));
    bmpDrawGray8WithColor(ArrowX+18, ArrowY+36, "/arrowD.bmp", (key&0x08)?RGB565_WHITE:RGB565(102, 204, 255));
    bmpDrawGray8WithColor(ArrowX+20, ArrowY+20, "/arrowM.bmp", (key&0x10)?RGB565_WHITE:RGB565(102, 204, 255));
}

#define BATX 160-13
#define BATY 0
void showBattery(uint8_t level)
{
  static uint8_t lastLevel = 0xFF;
  if (level == lastLevel) return;
  lastLevel = level;

  const uint8_t x = BATX;
  const uint8_t y = BATY;

  if (level > 5) {
    gfx->fillRect(x, y, 12, 8, RGB565_BLACK);
    return;
  }

  BMPGray8 bmp;
  if (!loadBMPGray8("/battery.bmp", bmp)) {
    Serial.println("Failed to load battery.bmp");
    return;
  }

  if (bmp.width < 10 || bmp.height < 6 || !bmp.pixels) {
    free(bmp.pixels);
    return;
  }

  // 需要隐藏的电池格起始索引
  const struct { uint8_t x0, y0, x1, y1; } cells[4] = {
    {2, 2, 3, 5},
    {4, 2, 5, 5},
    {6, 2, 7, 5},
    {8, 2, 9, 5},
  };

  int hideStart = 0;
  if (level == 0) hideStart = 0;
  else if (level == 1) hideStart = 1;
  else if (level <= 4) hideStart = level;
  else hideStart = 4;

  for (int i = hideStart; i < 4; ++i) {
    for (uint8_t yy = cells[i].y0; yy <= cells[i].y1; ++yy) {
      for (uint8_t xx = cells[i].x0; xx <= cells[i].x1; ++xx) {
        bmp.pixels[yy * bmp.width + xx] = 0; // 灰度0 = 黑色
      }
    }
  }

  gfx->drawGrayWithColorBitmap(x, y, bmp.pixels, 0xFFFF, bmp.width, bmp.height);
  free(bmp.pixels);
}

void showLinkState(bool sw)
{
  bmpDrawGray8WithColor(160-16,22,sw?"/link.bmp":"/download.bmp", RGB565_WHITE);
}