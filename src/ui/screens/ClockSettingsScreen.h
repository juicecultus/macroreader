#ifndef CLOCK_SETTINGS_SCREEN_H
#define CLOCK_SETTINGS_SCREEN_H

#include <Arduino.h>

#include "../../core/EInkDisplay.h"
#include "../../rendering/TextRenderer.h"
#include "Screen.h"

class Buttons;
class UIManager;

class ClockSettingsScreen : public Screen {
 public:
  ClockSettingsScreen(EInkDisplay& display, TextRenderer& renderer, UIManager& uiManager);

  void begin() override;
  void handleButtons(Buttons& buttons) override;
  void activate() override;
  void show() override;
  void shutdown() override {}

 private:
  EInkDisplay& display;
  TextRenderer& textRenderer;
  UIManager& uiManager;

  int selectedIndex = 0;

  static constexpr int ITEM_COUNT = 2;

  void render();
  void selectNext();
  void selectPrev();
  void activateSelection();

  String getItemName(int index);
  String getItemValue(int index);
};

#endif
