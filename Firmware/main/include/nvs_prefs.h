#pragma once
#include <string>
#include <stdint.h>
#include <stdbool.h>
#include "nvs_flash.h"
#include "nvs.h"

// Drop-in C++ replacement for Arduino Preferences, backed by ESP-IDF NVS.
// Uses std::string instead of Arduino String.
class NvsPrefs {
public:
    NvsPrefs() : _handle(0) {}
    ~NvsPrefs() { if (_handle) nvs_close(_handle); }

    bool begin(const char* ns, bool readOnly = false);
    void end();

    uint32_t    getUInt(const char* key, uint32_t def = 0);
    bool        putUInt(const char* key, uint32_t value);

    bool        getBool(const char* key, bool def = false);
    bool        putBool(const char* key, bool value);

    std::string getString(const char* key, const char* def = "");
    bool        putString(const char* key, const char* value);
    bool        putString(const char* key, const std::string& value);

    bool        isKey(const char* key);
    bool        remove(const char* key);
    bool        clear();

private:
    nvs_handle_t _handle;

    bool _commit();
};
