#include "one_gfx.h"
// #include <font/FreeMono9pt7b.h>   // 引入字体头文件
#include "BmpClass.h"

static BmpClass bmpClass;

#define GFX_BL 10
Arduino_DataBus *bus = new Arduino_ESP32SPI(2 /* DC */, 4 /* CS */, 1 /* SCK */,0 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_ST7735(bus, 3 /* RST */, 1 /* rotation */, false /* IPS */,80 /* width */, 160 /* height */,24 /* col offset 1 */, 0 /* row offset 1 */,24 /* col offset 2 */, 0 /* row offset 2 */);
GUIManager gui(gfx);  // 创建 GUI 管理器对象，传入 gfx 实例

void initIcons(void);
void refreshIcons(void);
void GUIElement::draw(){
    if (isShow) {
        gui.bmpDrawGray8WithColor(this);
        
        // 如果是当前选中元素则绘制选择框
        if (gui.currentSelected == this) {
            //gui.drawSelectionBox(this);
        }
    }
}

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

  // 双线性插值缩放
  for (uint16_t y = 0; y < dstH; ++y) {
    float srcY = y / scale;
    int y0 = floor(srcY);
    int y1 = min(y0 + 1, height - 1);
    float yLerp = srcY - y0;

    for (uint16_t x = 0; x < dstW; ++x) {
      float srcX = x / scale;
      int x0 = floor(srcX);
      int x1 = min(x0 + 1, width - 1);
      float xLerp = srcX - x0;

      // 取四个邻近像素
      uint8_t p00 = rawBuf[y0 * width + x0];
      uint8_t p01 = rawBuf[y0 * width + x1];
      uint8_t p10 = rawBuf[y1 * width + x0];
      uint8_t p11 = rawBuf[y1 * width + x1];

      // 水平插值
      float top = p00 + (p01 - p00) * xLerp;
      float bottom = p10 + (p11 - p10) * xLerp;

      // 垂直插值
      float value = top + (bottom - top) * yLerp;

      outBuf[y * dstW + x] = (uint8_t)value;
    }
  }

  free(rawBuf);

  outBmp.pixels = outBuf;
  outBmp.width = dstW;
  outBmp.height = dstH;
  return true;

}


void GUIManager::bmpDrawGray8WithColor(GUIElement* picObj)
{
    if (!picObj || !picObj->info.path) {
        Serial.println("Invalid element");
        return;
    }

    if (!picObj->isRefresh)
        return;

    picObj->isRefresh = false;

    if (picObj->isLoad && picObj->info.buffer) {
        gfx->drawGrayWithColorBitmap(picObj->x, picObj->y, picObj->info.buffer, picObj->isSelected?RGB565_CYAN:picObj->color,
                                     picObj->info.width, picObj->info.height);
        return;
    }

    BMPGray8 bmp;
    if (picObj->info.buffer) {
        bmp.pixels = (uint8_t*)picObj->info.buffer;
    }

    if (!loadBMPGray8(picObj->info.path, bmp, picObj->info.scale, picObj->info.bufferSize)) {
        Serial.println("Failed to load BMP");
        return;
    }

    picObj->info.width = bmp.width;
    picObj->info.height = bmp.height;

    if(picObj->isShow){
      gfx->drawGrayWithColorBitmap(picObj->x, picObj->y, bmp.pixels, picObj->isSelected?RGB565_CYAN:picObj->color,
                                 picObj->info.width, picObj->info.height);
    }

    if (!picObj->info.buffer || bmp.pixels != picObj->info.buffer) {
        free(bmp.pixels);
        bmp.pixels = nullptr;
    } else {
        picObj->isLoad = 1;
    }
}

void GUIManager::bmpDrawGray8WithColor(int16_t x, int16_t y, const char* path, uint16_t color, float scale)
{
    GUIElement temp(x, y, path,0 , color, scale);
    bmpDrawGray8WithColor(&temp);
}

void GUIManager::bmpStringWithColor(uint8_t Stax, uint8_t Stay, const String str,uint16_t color)
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

    gfx->drawGrayWithColorBitmap(Stax, Stay, fontBuf, color, FONT_WIDTH, FONT_HEIGHT);
    Stax += 9; // 每个字符之间的间隔
  }

  fontFile.close(); // 关闭文件
}

void GUIManager::lcd_init(){
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
  }
}

void GUIManager::drawSelectionBox(GUIElement* elem){
    if (!elem) return;
    
    // 绘制选择框 (x-1, y-1, x+w, y+h)
    gfx->drawRect(
        elem->x, 
        elem->y, 
        elem->info.width,  // 宽度包含两侧边框
        elem->info.height, // 高度包含两侧边框
        RGB565_RED        // 红色
    );
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

    gui.bmpStringWithColor(22, y, String(signalIO.Info[i].label),RGB565_WHITE);
  }
}
