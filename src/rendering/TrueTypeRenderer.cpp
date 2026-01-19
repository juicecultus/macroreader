#include "TrueTypeRenderer.h"

#include "../core/EInkDisplay.h"

#ifdef USE_M5UNIFIED
#include <bb_truetype.h>

// Static instance pointer for callback
TrueTypeRenderer* TrueTypeRenderer::activeInstance = nullptr;

// bb_truetype instance (static to avoid repeated construction)
static bb_truetype g_ttf;

TrueTypeRenderer::TrueTypeRenderer(EInkDisplay& display) : display(display) {
  Serial.printf("[%lu] TrueTypeRenderer: Constructor, free heap: %d\n", millis(), ESP.getFreeHeap());
}

TrueTypeRenderer::~TrueTypeRenderer() {
  closeFont();
}

void TrueTypeRenderer::drawLineCallback(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t color) {
  if (!activeInstance) return;
  
  uint8_t* fb = activeInstance->display.getFrameBuffer();
  if (!fb) return;
  
  const int fbWidth = EInkDisplay::DISPLAY_WIDTH;
  const int fbHeight = EInkDisplay::DISPLAY_HEIGHT;
  const int fbWidthBytes = EInkDisplay::DISPLAY_WIDTH_BYTES;
  
  // Determine if we're drawing black (0) or white (1)
  // bb_truetype passes color as set by setTextColor - we use 0 for black
  const bool drawBlack = (color == 0);
  
  // Horizontal line (most common for fill) - optimized path
  if (y0 == y1) {
    if (y0 < 0 || y0 >= fbHeight) return;
    if (x0 > x1) { int16_t t = x0; x0 = x1; x1 = t; }
    if (x1 < 0 || x0 >= fbWidth) return;
    if (x0 < 0) x0 = 0;
    if (x1 >= fbWidth) x1 = fbWidth - 1;
    
    for (int16_t x = x0; x <= x1; x++) {
      int idx = y0 * fbWidthBytes + (x / 8);
      uint8_t mask = 0x80 >> (x & 7);
      if (drawBlack) {
        fb[idx] &= ~mask;  // Clear bit = black
      } else {
        fb[idx] |= mask;   // Set bit = white
      }
    }
    return;
  }
  
  // Vertical line - also common
  if (x0 == x1) {
    if (x0 < 0 || x0 >= fbWidth) return;
    if (y0 > y1) { int16_t t = y0; y0 = y1; y1 = t; }
    if (y1 < 0 || y0 >= fbHeight) return;
    if (y0 < 0) y0 = 0;
    if (y1 >= fbHeight) y1 = fbHeight - 1;
    
    uint8_t mask = 0x80 >> (x0 & 7);
    int xByte = x0 / 8;
    for (int16_t y = y0; y <= y1; y++) {
      int idx = y * fbWidthBytes + xByte;
      if (drawBlack) {
        fb[idx] &= ~mask;
      } else {
        fb[idx] |= mask;
      }
    }
    return;
  }
  
  // Arbitrary line - Bresenham's algorithm
  int16_t dx = abs(x1 - x0);
  int16_t dy = -abs(y1 - y0);
  int16_t sx = (x0 < x1) ? 1 : -1;
  int16_t sy = (y0 < y1) ? 1 : -1;
  int16_t err = dx + dy;
  
  while (true) {
    // Draw pixel at (x0, y0)
    if (x0 >= 0 && x0 < fbWidth && y0 >= 0 && y0 < fbHeight) {
      int idx = y0 * fbWidthBytes + (x0 / 8);
      uint8_t mask = 0x80 >> (x0 & 7);
      if (drawBlack) {
        fb[idx] &= ~mask;
      } else {
        fb[idx] |= mask;
      }
    }
    
    if (x0 == x1 && y0 == y1) break;
    int16_t e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

bool TrueTypeRenderer::loadFont(const char* path) {
  Serial.printf("[%lu] TrueTypeRenderer: Loading font '%s'\n", millis(), path);
  printMemoryStats();
  
  closeFont();  // Close any previously open font
  
  fontFile = SD.open(path, FILE_READ);
  if (!fontFile) {
    Serial.printf("[%lu] TrueTypeRenderer: Failed to open font file\n", millis());
    return false;
  }
  
  Serial.printf("[%lu] TrueTypeRenderer: Font file opened, size: %d bytes\n", millis(), fontFile.size());
  
  // Set up the TTF renderer
  // Note: setTtfFile returns 1 on success, 0 on failure
  uint8_t result = g_ttf.setTtfFile(fontFile, 0);  // 0 = don't verify checksum (faster)
  if (result == 0) {
    Serial.printf("[%lu] TrueTypeRenderer: setTtfFile failed\n", millis());
    fontFile.close();
    return false;
  }
  
  // Set up our draw callback
  activeInstance = this;
  g_ttf.setTtfDrawLine(drawLineCallback);
  
  // Set default character size
  g_ttf.setCharacterSize(charSize);
  
  // Set text color (0 = black for our callback)
  g_ttf.setTextColor(textColor, textColor);
  
  fontLoaded = true;
  Serial.printf("[%lu] TrueTypeRenderer: Font loaded successfully\n", millis());
  printMemoryStats();
  
  return true;
}

void TrueTypeRenderer::closeFont() {
  if (fontLoaded) {
    g_ttf.end();
    fontFile.close();
    fontLoaded = false;
    if (activeInstance == this) {
      activeInstance = nullptr;
    }
    Serial.printf("[%lu] TrueTypeRenderer: Font closed\n", millis());
  }
}

void TrueTypeRenderer::setCharacterSize(uint16_t size) {
  charSize = size;
  if (fontLoaded) {
    g_ttf.setCharacterSize(size);
  }
}

void TrueTypeRenderer::setTextColor(uint8_t color) {
  textColor = color;
  if (fontLoaded) {
    g_ttf.setTextColor(color, color);
  }
}

void TrueTypeRenderer::drawText(int16_t x, int16_t y, const char* text) {
  if (!fontLoaded) {
    Serial.printf("[%lu] TrueTypeRenderer: drawText called but no font loaded\n", millis());
    return;
  }
  
  activeInstance = this;
  g_ttf.textDraw(x, y, text);
}

uint16_t TrueTypeRenderer::getStringWidth(const char* text) {
  if (!fontLoaded) {
    return 0;
  }
  return g_ttf.getStringWidth(text);
}

void TrueTypeRenderer::printMemoryStats() {
  Serial.printf("[%lu] Memory - Free heap: %d, Min free: %d, PSRAM free: %d\n",
                millis(), ESP.getFreeHeap(), ESP.getMinFreeHeap(),
                ESP.getFreePsram());
}

#else
// Stub implementation for non-M5UNIFIED builds

TrueTypeRenderer* TrueTypeRenderer::activeInstance = nullptr;

TrueTypeRenderer::TrueTypeRenderer(EInkDisplay& display) : display(display) {}
TrueTypeRenderer::~TrueTypeRenderer() {}
bool TrueTypeRenderer::loadFont(const char* path) { return false; }
void TrueTypeRenderer::closeFont() {}
void TrueTypeRenderer::setCharacterSize(uint16_t size) { charSize = size; }
void TrueTypeRenderer::setTextColor(uint8_t color) { textColor = color; }
void TrueTypeRenderer::drawText(int16_t x, int16_t y, const char* text) {}
uint16_t TrueTypeRenderer::getStringWidth(const char* text) { return 0; }
void TrueTypeRenderer::printMemoryStats() {}
void TrueTypeRenderer::drawLineCallback(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t color) {}

#endif
