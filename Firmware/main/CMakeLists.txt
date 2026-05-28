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
        "src/bp32_platform.cpp"
        "src/dns_server.c")

set(requires "bluepad32" "btstack" "esp_http_server" "json" "nvs_flash" "esp_netif" "esp_wifi" "esp_coex" "app_update" "esp_partition" "mdns" "esp_timer" "driver")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "." "include"
                    REQUIRES "${requires}"
                    EMBED_FILES "index.html")
