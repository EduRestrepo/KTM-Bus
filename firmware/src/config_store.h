#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "types.h"

class ConfigStore {
public:
    void init();
    void loadConfig(SystemConfig& cfg);
    void saveConfig(const SystemConfig& cfg);
    void setDefaults(SystemConfig& cfg);

private:
    Preferences prefs;
};

extern ConfigStore configStore;
