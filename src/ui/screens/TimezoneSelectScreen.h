#ifndef TIMEZONE_SELECT_SCREEN_H
#define TIMEZONE_SELECT_SCREEN_H

#include <Arduino.h>

#include "../../core/EInkDisplay.h"
#include "../../rendering/TextRenderer.h"
#include "Screen.h"

class Buttons;
class UIManager;

class TimezoneSelectScreen : public Screen {
 public:
  TimezoneSelectScreen(EInkDisplay& display, TextRenderer& renderer, UIManager& uiManager);

  void begin() override;
  void handleButtons(Buttons& buttons) override;
  void activate() override;
  void show() override;
  void shutdown() override {}

 private:
  enum class Level {
    Continent,
    Country,
    City
  };

  EInkDisplay& display;
  TextRenderer& textRenderer;
  UIManager& uiManager;

  Level level = Level::Continent;
  int selectedIndex = 0;

  int selectedContinent = -1;
  int selectedCountry = -1;

  void render();
  void selectNext();
  void selectPrev();
  void activateSelection();
  void goBack();

  int getItemCount() const;
  String getTitle() const;
  String getItemLabel(int index) const;

  void saveTimezoneSelection(const String& continent, const String& country, const String& city, int tzOffsetHours);
};

#endif
