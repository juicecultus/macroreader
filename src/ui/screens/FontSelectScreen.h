#ifndef FONTSELECTSCREEN_H
#define FONTSELECTSCREEN_H

#include "../../core/EInkDisplay.h"
#include "../../rendering/TextRenderer.h"
#include "../../rendering/TrueTypeRenderer.h"
#include "Screen.h"

class Buttons;
class UIManager;

class FontSelectScreen : public Screen {
 public:
  FontSelectScreen(EInkDisplay& display, TextRenderer& renderer, UIManager& uiManager);

  void begin() override;
  void handleButtons(Buttons& buttons) override;
  void activate() override;
  void show() override;
  void shutdown() override {}

 private:
  EInkDisplay& display;
  TextRenderer& textRenderer;
  UIManager& uiManager;

  static constexpr int MAX_FONTS = 20;
  static constexpr int ITEMS_PER_PAGE = 10;
  
  String fontFiles[MAX_FONTS];
  int fontCount = 0;
  int selectedIndex = 0;
  int scrollOffset = 0;
  bool inPreviewMode = false;

  void scanFontsDirectory();
  void renderFontList();
  void selectFont();
  void showFontPreview();
};

#endif
