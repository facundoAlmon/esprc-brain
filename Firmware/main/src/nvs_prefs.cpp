#include "nvs_prefs.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "NvsPrefs";

bool NvsPrefs::begin(const char* ns, bool readOnly) {
    nvs_open_mode_t mode = readOnly ? NVS_READONLY : NVS_READWRITE;
    esp_err_t err = nvs_open(ns, mode, &_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open(%s) failed: %s", ns, esp_err_to_name(err));
        _handle = 0;
        return false;
    }
    return true;
}

void NvsPrefs::end() {
    if (_handle) {
        nvs_close(_handle);
        _handle = 0;
    }
}

bool NvsPrefs::_commit() {
    esp_err_t err = nvs_commit(_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_commit failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

uint32_t NvsPrefs::getUInt(const char* key, uint32_t def) {
    uint32_t val = def;
    nvs_get_u32(_handle, key, &val);
    return val;
}

bool NvsPrefs::putUInt(const char* key, uint32_t value) {
    esp_err_t err = nvs_set_u32(_handle, key, value);
    if (err != ESP_OK) { ESP_LOGE(TAG, "putUInt(%s) failed: %s", key, esp_err_to_name(err)); return false; }
    return _commit();
}

bool NvsPrefs::getBool(const char* key, bool def) {
    uint8_t val = def ? 1 : 0;
    nvs_get_u8(_handle, key, &val);
    return val != 0;
}

bool NvsPrefs::putBool(const char* key, bool value) {
    esp_err_t err = nvs_set_u8(_handle, key, value ? 1 : 0);
    if (err != ESP_OK) { ESP_LOGE(TAG, "putBool(%s) failed: %s", key, esp_err_to_name(err)); return false; }
    return _commit();
}

std::string NvsPrefs::getString(const char* key, const char* def) {
    size_t required = 0;
    esp_err_t err = nvs_get_str(_handle, key, nullptr, &required);
    if (err != ESP_OK || required == 0) return def ? def : "";

    std::string result(required, '\0');
    err = nvs_get_str(_handle, key, &result[0], &required);
    if (err != ESP_OK) return def ? def : "";

    // Remove trailing null that nvs_get_str includes in `required`
    if (!result.empty() && result.back() == '\0') result.pop_back();
    return result;
}

bool NvsPrefs::putString(const char* key, const char* value) {
    esp_err_t err = nvs_set_str(_handle, key, value ? value : "");
    if (err != ESP_OK) { ESP_LOGE(TAG, "putString(%s) failed: %s", key, esp_err_to_name(err)); return false; }
    return _commit();
}

bool NvsPrefs::putString(const char* key, const std::string& value) {
    return putString(key, value.c_str());
}

bool NvsPrefs::isKey(const char* key) {
    // Try u32 first, then str — if either succeeds the key exists
    uint32_t dummy_u = 0;
    if (nvs_get_u32(_handle, key, &dummy_u) == ESP_OK) return true;
    uint8_t dummy_b = 0;
    if (nvs_get_u8(_handle, key, &dummy_b) == ESP_OK) return true;
    size_t sz = 0;
    if (nvs_get_str(_handle, key, nullptr, &sz) == ESP_OK) return true;
    return false;
}

bool NvsPrefs::remove(const char* key) {
    esp_err_t err = nvs_erase_key(_handle, key);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "remove(%s) failed: %s", key, esp_err_to_name(err));
        return false;
    }
    return _commit();
}

bool NvsPrefs::clear() {
    esp_err_t err = nvs_erase_all(_handle);
    if (err != ESP_OK) { ESP_LOGE(TAG, "clear() failed: %s", esp_err_to_name(err)); return false; }
    return _commit();
}
