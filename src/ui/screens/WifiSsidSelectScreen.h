#ifndef WIFI_SSID_SELECT_SCREEN_H
#define WIFI_SSID_SELECT_SCREEN_H

#include <Arduino.h>

#include <vector>

#include "../../core/EInkDisplay.h"
#include "../../rendering/TextRenderer.h"
#include "Screen.h"

class Buttons;
class UIManager;

class WifiSsidSelectScreen : public Screen {
 public:
  WifiSsidSelectScreen(EInkDisplay& display, TextRenderer& renderer, UIManager& uiManager);

  void begin() override;
  void handleButtons(Buttons& buttons) override;
  void activate() override;
  void show() override;
  void shutdown() override {}

 private:
  EInkDisplay& display;
  TextRenderer& textRenderer;
  UIManager& uiManager;

  std::vector<String> ssids;
  int selectedIndex = 0;
  int scrollOffset = 0;
  bool scanning = false;

  void scanNetworks();
  void render();
  void selectNext();
  void selectPrev();
  void confirm();
};

#endif
