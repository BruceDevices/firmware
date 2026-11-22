#pragma once

#include <ArduinoJson.h>

enum class ConfigReadResult { Loaded, Missing, Unreadable };

ConfigReadResult loadConfigFile(const char *path, JsonDocument &document, bool useSd);
void saveConfigFile(const char *path, const JsonDocument &document, bool useSd);
void backupConfigFile(const char *path, bool useSd);
