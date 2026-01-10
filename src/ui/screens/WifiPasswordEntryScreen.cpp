#include "WifiPasswordEntryScreen.h"

#include <resources/fonts/FontManager.h>
#include <resources/fonts/other/MenuFontSmall.h>
#include <resources/fonts/other/MenuHeader.h>

#include "../../core/Settings.h"
#include "../UIManager.h"

static const char* kPwChoices = "[OK][DEL] abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.@+/\\:";

WifiPasswordEntryScreen::WifiPasswordEntryScreen(EInkDisplay& display, TextRenderer& renderer, UIManager& uiManager)
    : display(display), textRenderer(renderer), uiManager(uiManager) {}

void WifiPasswordEntryScreen::begin() {
  loadSettings();
}

void WifiPasswordEntryScreen::activate() {
  loadSettings();
  editOriginal = wifiPass;
  editBuffer = wifiPass;
  choiceIndex = 0;
}

void WifiPasswordEntryScreen::handleButtons(Buttons& buttons) {
  int choicesLen = (int)strlen(kPwChoices);

  if (buttons.isPressed(Buttons::BACK)) {
    editBuffer = editOriginal;
    uiManager.showScreen(UIManager::ScreenId::WifiSettings);
  } else if (buttons.isPressed(Buttons::LEFT)) {
    choiceIndex++;
    if (choiceIndex >= choicesLen)
      choiceIndex = 0;
    show();
  } else if (buttons.isPressed(Buttons::RIGHT)) {
    choiceIndex--;
    if (choiceIndex < 0)
      choiceIndex = choicesLen - 1;
    show();
  } else if (buttons.isPressed(Buttons::CONFIRM)) {
    choose();
  }
}

void WifiPasswordEntryScreen::show() {
  render();
  display.displayBuffer(EInkDisplay::FAST_REFRESH);
}

void WifiPasswordEntryScreen::loadSettings() {
  Settings& s = uiManager.getSettings();
  wifiPass = s.getString(String("wifi.pass"));
}

void WifiPasswordEntryScreen::saveSettings() {
  Settings& s = uiManager.getSettings();
  s.setString(String("wifi.pass"), wifiPass);
  if (!s.save()) {
    Serial.println("WifiPasswordEntryScreen: Failed to write settings.cfg");
  }
}

void WifiPasswordEntryScreen::choose() {
  // First 4 chars encode [OK]
  if (choiceIndex >= 0 && choiceIndex <= 3) {
    wifiPass = editBuffer;
    saveSettings();
    uiManager.showScreen(UIManager::ScreenId::WifiSettings);
    return;
  }
  // Next 5 chars encode [DEL]
  if (choiceIndex >= 4 && choiceIndex <= 8) {
    if (editBuffer.length() > 0) {
      editBuffer.remove(editBuffer.length() - 1);
    }
    show();
    return;
  }

  char c = kPwChoices[choiceIndex];
  if (c != '[' && c != ']' && c != 'O' && c != 'K' && c != 'D' && c != 'E' && c != 'L') {
    if (editBuffer.length() < 64) {
      editBuffer += c;
    }
  }
  show();
}

void WifiPasswordEntryScreen::render() {
  display.clearScreen(0xFF);
  textRenderer.setTextColor(TextRenderer::COLOR_BLACK);
  textRenderer.setFont(getTitleFont());

  textRenderer.setFrameBuffer(display.getFrameBuffer());
  textRenderer.setBitmapType(TextRenderer::BITMAP_BW);

  {
    const char* title = "WiFi Password";
    int16_t x1, y1;
    uint16_t w, h;
    textRenderer.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
    int16_t centerX = (480 - (int)w) / 2;
    textRenderer.setCursor(centerX, 75);
    textRenderer.print(title);
  }

  textRenderer.setFont(getMainFont());

  {
    String shown;
    for (int i = 0; i < editBuffer.length() && i < 32; ++i)
      shown += "*";
    if (editBuffer.length() > 32)
      shown += "...";

    String line = String("Password: ") + shown;
    int16_t x1, y1;
    uint16_t w, h;
    textRenderer.getTextBounds(line.c_str(), 0, 0, &x1, &y1, &w, &h);
    int16_t centerX = (480 - (int)w) / 2;
    textRenderer.setCursor(centerX, 200);
    textRenderer.print(line);
  }

  {
    char c = kPwChoices[choiceIndex];
    String choice;
    if (choiceIndex >= 0 && choiceIndex <= 3) {
      choice = "[OK]";
    } else if (choiceIndex >= 4 && choiceIndex <= 8) {
      choice = "[DEL]";
    } else {
      choice = String("[") + String(c) + String("]");
    }

    int16_t x1, y1;
    uint16_t w, h;
    textRenderer.getTextBounds(choice.c_str(), 0, 0, &x1, &y1, &w, &h);
    int16_t centerX = (480 - (int)w) / 2;
    textRenderer.setCursor(centerX, 260);
    textRenderer.print(choice);
  }

  {
    textRenderer.setFont(&MenuFontSmall);
    textRenderer.setCursor(20, 780);
    textRenderer.print("Left/Right: Choose  OK: Select  Back: Cancel");
  }
}
