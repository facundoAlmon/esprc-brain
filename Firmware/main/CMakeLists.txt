set(srcs
        "main.c"
        "src/sketch.cpp"
        "src/gamepadHandler.cpp"
        "src/ledStripHandler.cpp"
        "src/webServerHandler.cpp"
        "src/actuators.cpp"
        "src/state.cpp"
        "src/ProgramManager.cpp")

set(requires "bluepad32" "bluepad32_arduino" "arduino" "btstack" "esp_http_server" "json" "nvs_flash" "esp_netif" "esp_wifi" "esp_coex")
idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "." "include"
                    REQUIRES "${requires}"
                    EMBED_FILES "index.html")



#idf_component_register(SRCS "${srcs}"
#                    INCLUDE_DIRS "." "include"
#                    REQUIRES "${requires}"
#                    #"esp32-camera"
#                    EMBED_FILES "index.html")