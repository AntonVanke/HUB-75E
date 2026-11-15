#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <esp_system.h>

namespace CrashLog {
bool begin(Preferences& prefs,esp_reset_reason_t reset,const char* version);
void sample(int theme,uint32_t generation);
void renderSample(int theme,uint32_t generation);
void requestBegin(const char* route,unsigned method);
void requestEnd();
String json(Preferences& prefs);
}
