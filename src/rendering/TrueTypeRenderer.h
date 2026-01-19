#ifndef TRUETYPE_RENDERER_H
#define TRUETYPE_RENDERER_H

#include <Arduino.h>
#include <SD.h>

class EInkDisplay;

class TrueTypeRenderer {
 public:
  TrueTypeRenderer(EInkDisplay& display);
  ~TrueTypeRenderer();

  // Load a TTF font from SD card. Returns true on success.
  bool loadFont(const char* path);

  // Close the currently loaded font file
  void closeFont();

  // Check if a font is currently loaded
  bool isFontLoaded() const { return fontLoaded; }

  // Set character size in pixels (height from top to below baseline)
  void setCharacterSize(uint16_t size);

  // Set text color (0 = black, 1 = white for 1-bit e-ink)
  void setTextColor(uint8_t color);

  // Draw text at the given position
  void drawText(int16_t x, int16_t y, const char* text);

  // Get the width of a string in pixels (for layout/centering)
  uint16_t getStringWidth(const char* text);

  // Get current character size
  uint16_t getCharacterSize() const { return charSize; }

  // Memory stats for monitoring
  static void printMemoryStats();

 private:
  EInkDisplay& display;
  File fontFile;
  bool fontLoaded = false;
  uint16_t charSize = 24;
  uint8_t textColor = 0;  // 0 = black

  // DrawLine callback needs access to display framebuffer
  static TrueTypeRenderer* activeInstance;
  static void drawLineCallback(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t color);
};

#endif
