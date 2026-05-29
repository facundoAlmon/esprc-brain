set(srcs
        "main.c"
        "src/sketch.cpp"
        "src/gamepadHandler.cpp"
        "src/ledStripHandler.cpp"
        "src/webServerHandler.cpp"
        "src/actuators.cpp"
        "src/state.cpp"
        "src/ProgramManager.cpp"
        "src/nvs_prefs.cpp"
        "src/dns_server.c")

set(requires "esp_http_server" "json" "nvs_flash" "esp_netif" "esp_wifi" "esp_coex"
             "app_update" "esp_partition" "mdns" "esp_timer" "driver")

if(CONFIG_IDF_TARGET_ESP32)
    list(APPEND srcs "src/hid_gamepad.cpp" "src/esp_hid_gap.c")
    list(APPEND requires "bt" "esp_hid")
else()
    list(APPEND srcs "src/hid_gamepad_stub.cpp")
endif()

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "." "include"
                    REQUIRES "${requires}"
                    EMBED_FILES "index.html")
