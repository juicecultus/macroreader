#include "FontSelectScreen.h"

#include <SD.h>

#include "../../core/Buttons.h"
#include "../../core/Settings.h"
#include "../../rendering/TrueTypeRenderer.h"
#include "../UIManager.h"
#include <resources/fonts/FontManager.h>
#include <resources/fonts/other/MenuFontBig.h>
#include <resources/fonts/other/MenuHeader.h>

FontSelectScreen::FontSelectScreen(EInkDisplay& display, TextRenderer& renderer, UIManager& uiManager)
    : display(display), textRenderer(renderer), uiManager(uiManager) {}

void FontSelectScreen::begin() {
  scanFontsDirectory();
}

void FontSelectScreen::handleButtons(Buttons& buttons) {
  if (buttons.isPressed(Buttons::BACK)) {
    if (inPreviewMode) {
      // Return from preview to list
      inPreviewMode = false;
      show();
    } else {
      uiManager.showScreen(UIManager::ScreenId::Settings);
    }
  } else if (buttons.isPressed(Buttons::LEFT)) {
    if (inPreviewMode) return;  // Ignore navigation in preview mode
    // Move down
    if (fontCount > 0) {
      selectedIndex++;
      if (selectedIndex >= fontCount + 1) {  // +1 for "Built-in" option
        selectedIndex = 0;
      }
      // Adjust scroll if needed
      if (selectedIndex >= scrollOffset + ITEMS_PER_PAGE) {
        scrollOffset = selectedIndex - ITEMS_PER_PAGE + 1;
      } else if (selectedIndex < scrollOffset) {
        scrollOffset = selectedIndex;
      }
      show();
    }
  } else if (buttons.isPressed(Buttons::RIGHT)) {
    if (inPreviewMode) return;  // Ignore navigation in preview mode
    // Move up
    if (fontCount > 0) {
      selectedIndex--;
      if (selectedIndex < 0) {
        selectedIndex = fontCount;  // Wrap to last (fontCount = number of TTF files, index 0 is "Built-in")
      }
      // Adjust scroll if needed
      if (selectedIndex < scrollOffset) {
        scrollOffset = selectedIndex;
      } else if (selectedIndex >= scrollOffset + ITEMS_PER_PAGE) {
        scrollOffset = selectedIndex - ITEMS_PER_PAGE + 1;
      }
      show();
    }
  } else if (buttons.isPressed(Buttons::CONFIRM)) {
    if (inPreviewMode) {
      // Confirm selection from preview
      selectFont();
    } else if (selectedIndex > 0) {
      // Show preview for TTF fonts
      inPreviewMode = true;
      showFontPreview();
    } else {
      // Built-in font - select directly (no preview needed)
      selectFont();
    }
  }
}

void FontSelectScreen::activate() {
  selectedIndex = 0;
  scrollOffset = 0;
  inPreviewMode = false;
  scanFontsDirectory();
  
  // Try to find current selection
  Settings& s = uiManager.getSettings();
  String currentFont;
  if (s.getString(String("settings.customFont"), currentFont) && currentFont.length() > 0) {
    // Find this font in the list
    for (int i = 0; i < fontCount; i++) {
      if (fontFiles[i] == currentFont) {
        selectedIndex = i + 1;  // +1 because index 0 is "Built-in"
        break;
      }
    }
  }
  
  // Adjust scroll to show selected item
  if (selectedIndex >= ITEMS_PER_PAGE) {
    scrollOffset = selectedIndex - ITEMS_PER_PAGE + 1;
  }
}

void FontSelectScreen::show() {
  renderFontList();
  display.displayBuffer(EInkDisplay::FAST_REFRESH);
}

void FontSelectScreen::scanFontsDirectory() {
  fontCount = 0;
  
  File fontsDir = SD.open("/fonts");
  if (!fontsDir || !fontsDir.isDirectory()) {
    Serial.printf("[%lu] FontSelectScreen: /fonts directory not found\n", millis());
    return;
  }
  
  while (fontCount < MAX_FONTS) {
    File entry = fontsDir.openNextFile();
    if (!entry) break;
    
    String name = entry.name();
    entry.close();
    
    // Skip hidden/system files (starting with . or _)
    if (name.length() > 0 && (name[0] == '.' || name[0] == '_')) {
      continue;
    }
    
    // Check for .ttf extension (case insensitive)
    if (name.length() > 4) {
      String ext = name.substring(name.length() - 4);
      ext.toLowerCase();
      if (ext == ".ttf") {
        fontFiles[fontCount] = name;
        fontCount++;
        Serial.printf("[%lu] FontSelectScreen: Found font: %s\n", millis(), name.c_str());
      }
    }
  }
  
  fontsDir.close();
  Serial.printf("[%lu] FontSelectScreen: Found %d TTF fonts\n", millis(), fontCount);
}

void FontSelectScreen::renderFontList() {
  display.clearScreen(0xFF);
  textRenderer.setTextColor(TextRenderer::COLOR_BLACK);
  textRenderer.setFrameBuffer(display.getFrameBuffer());
  textRenderer.setBitmapType(TextRenderer::BITMAP_BW);
  
  const int16_t pageW = (int16_t)EInkDisplay::DISPLAY_WIDTH;
  
  // Title
  textRenderer.setFont(getTitleFont());
  {
    const char* title = "Select Font";
    int16_t x1, y1;
    uint16_t w, h;
    textRenderer.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
    int16_t centerX = (pageW - (int)w) / 2;
    textRenderer.setCursor(centerX, 60);
    textRenderer.print(title);
  }
  
  textRenderer.setFont(getMainFont());
  
  const int lineHeight = 32;
  const int startY = 120;
  const int marginX = 20;
  
  // Total items = 1 (Built-in) + fontCount
  int totalItems = 1 + fontCount;
  
  for (int i = 0; i < ITEMS_PER_PAGE && (scrollOffset + i) < totalItems; i++) {
    int itemIndex = scrollOffset + i;
    int16_t rowY = startY + i * lineHeight;
    
    String displayName;
    if (itemIndex == 0) {
      displayName = "Built-in (Bitmap)";
    } else {
      displayName = fontFiles[itemIndex - 1];
    }
    
    // Selection indicator
    if (itemIndex == selectedIndex) {
      displayName = "> " + displayName + " <";
    }
    
    textRenderer.setCursor(marginX, rowY);
    textRenderer.print(displayName);
  }
  
  // Show scroll indicator if needed
  if (totalItems > ITEMS_PER_PAGE) {
    String scrollInfo = String(scrollOffset + 1) + "-" + 
                        String(min(scrollOffset + ITEMS_PER_PAGE, totalItems)) + 
                        " of " + String(totalItems);
    int16_t x1, y1;
    uint16_t w, h;
    textRenderer.getTextBounds(scrollInfo.c_str(), 0, 0, &x1, &y1, &w, &h);
    textRenderer.setCursor((pageW - (int)w) / 2, startY + ITEMS_PER_PAGE * lineHeight + 20);
    textRenderer.print(scrollInfo);
  }
  
  // Instructions at bottom
  const char* instructions = "TAP to select, BACK to cancel";
  int16_t x1, y1;
  uint16_t w, h;
  textRenderer.getTextBounds(instructions, 0, 0, &x1, &y1, &w, &h);
  textRenderer.setCursor((pageW - (int)w) / 2, EInkDisplay::DISPLAY_HEIGHT - 40);
  textRenderer.print(instructions);
}

void FontSelectScreen::selectFont() {
  Settings& s = uiManager.getSettings();
  
  if (selectedIndex == 0) {
    // Built-in fonts - clear custom font setting
    s.setString(String("settings.customFont"), String(""));
    Serial.printf("[%lu] FontSelectScreen: Selected built-in fonts\n", millis());
  } else {
    // Custom TTF font
    String fontPath = "/fonts/" + fontFiles[selectedIndex - 1];
    s.setString(String("settings.customFont"), fontPath);
    Serial.printf("[%lu] FontSelectScreen: Selected font: %s\n", millis(), fontPath.c_str());
  }
  
  if (!s.save()) {
    Serial.printf("[%lu] FontSelectScreen: Failed to save settings\n", millis());
  }
  
  // Return to settings
  uiManager.showScreen(UIManager::ScreenId::Settings);
}

void FontSelectScreen::showFontPreview() {
  if (selectedIndex == 0) {
    // Built-in font - just show the list
    show();
    return;
  }
  
  String fontPath = "/fonts/" + fontFiles[selectedIndex - 1];
  Serial.printf("[%lu] FontSelectScreen: Previewing font: %s\n", millis(), fontPath.c_str());
  
  TrueTypeRenderer ttfRenderer(display);
  if (!ttfRenderer.loadFont(fontPath.c_str())) {
    Serial.printf("[%lu] FontSelectScreen: Failed to load font for preview\n", millis());
    show();
    return;
  }
  
  display.clearScreen(0xFF);
  
  const int16_t pageW = (int16_t)EInkDisplay::DISPLAY_WIDTH;
  
  // Title with built-in font
  textRenderer.setFrameBuffer(display.getFrameBuffer());
  textRenderer.setBitmapType(TextRenderer::BITMAP_BW);
  textRenderer.setFont(getTitleFont());
  textRenderer.setTextColor(TextRenderer::COLOR_BLACK);
  {
    String title = "Preview: " + fontFiles[selectedIndex - 1];
    int16_t x1, y1;
    uint16_t w, h;
    textRenderer.getTextBounds(title.c_str(), 0, 0, &x1, &y1, &w, &h);
    int16_t centerX = (pageW - (int)w) / 2;
    textRenderer.setCursor(centerX, 50);
    textRenderer.print(title);
  }
  
  // Render sample text with TTF at different sizes
  ttfRenderer.setTextColor(0);  // Black
  
  ttfRenderer.setCharacterSize(24);
  ttfRenderer.drawText(20, 130, "The quick brown fox jumps");
  ttfRenderer.drawText(20, 160, "over the lazy dog.");
  
  ttfRenderer.setCharacterSize(32);
  ttfRenderer.drawText(20, 230, "ABCDEFGHIJKLM");
  ttfRenderer.drawText(20, 270, "NOPQRSTUVWXYZ");
  
  ttfRenderer.setCharacterSize(28);
  ttfRenderer.drawText(20, 340, "abcdefghijklmnopqrstuvwxyz");
  ttfRenderer.drawText(20, 380, "0123456789 !@#$%^&*()");
  
  // Different sizes demo
  ttfRenderer.setCharacterSize(18);
  ttfRenderer.drawText(20, 450, "18pt: Small text for footnotes");
  
  ttfRenderer.setCharacterSize(36);
  ttfRenderer.drawText(20, 500, "36pt: Chapter titles");
  
  ttfRenderer.setCharacterSize(48);
  ttfRenderer.drawText(20, 570, "48pt: Headers");
  
  ttfRenderer.closeFont();
  
  // Instructions at bottom
  textRenderer.setFont(getMainFont());
  const char* instructions = "TAP to confirm, BACK to cancel";
  int16_t x1, y1;
  uint16_t w, h;
  textRenderer.getTextBounds(instructions, 0, 0, &x1, &y1, &w, &h);
  textRenderer.setCursor((pageW - (int)w) / 2, EInkDisplay::DISPLAY_HEIGHT - 40);
  textRenderer.print(instructions);
  
  display.displayBuffer(EInkDisplay::FAST_REFRESH);
}
