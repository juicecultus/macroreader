#ifndef SETTINGS_H
#define SETTINGS_H

#include <map>
#include <string>

#include "core/SDCardManager.h"

class Settings {
 public:
  explicit Settings(SDCardManager& sdManager);

  // Load from /microreader/settings.cfg or import legacy settings
  bool load();
  // Persist current settings to SD
  bool save();

  // Get/Set simple values
  bool getInt(const String& key, int& out) const;
  void setInt(const String& key, int v);

  String getString(const String& key, const String& def = String("")) const;
  void setString(const String& key, const String& value);

  // (Positions are stored per-file as `.pos` files; not part of consolidated settings)

 private:
  SDCardManager& sd;
  // Simple map of settings stored as string (use String for compatibility)
  std::map<String, String> kv;

  void parseSettingsBuffer(const char* buf);
};

#endif
