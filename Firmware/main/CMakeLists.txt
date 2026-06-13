# Auto-build webapp when sources change (guard: not available in IDF script mode)
if(NOT CMAKE_SCRIPT_MODE_FILE)
    file(GLOB_RECURSE WEBAPP_SRCS
        "${CMAKE_CURRENT_SOURCE_DIR}/../webapp/src/*.js"
        "${CMAKE_CURRENT_SOURCE_DIR}/../webapp/src/*.html"
        "${CMAKE_CURRENT_SOURCE_DIR}/../webapp/src/*.css")

    set(INDEX_HTML "${CMAKE_CURRENT_SOURCE_DIR}/index.html")

    add_custom_command(
        OUTPUT ${INDEX_HTML}
        COMMAND npm run build
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../webapp"
        DEPENDS ${WEBAPP_SRCS}
                "${CMAKE_CURRENT_SOURCE_DIR}/../webapp/gulpfile.js"
        COMMENT "Building webapp..."
        VERBATIM
    )

    add_custom_target(webapp_build DEPENDS ${INDEX_HTML})
endif()

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
        "src/dns_server.c"
        "src/espnow_manager.cpp")

if(CONFIG_BT_BLUEDROID_ENABLED)
    list(APPEND srcs "src/hid_gamepad.cpp" "src/esp_hid_gap.c")
else()
    list(APPEND srcs "src/hid_gamepad_stub.cpp")
endif()

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "." "include"
                    REQUIRES "esp_http_server" "nvs_flash" "esp_netif" "esp_wifi"
                             "esp_coex" "app_update" "esp_partition" "mdns" "esp_timer"
                             "driver" "esp_driver_gpio" "esp_driver_ledc" "esp_driver_mcpwm" "bt" "esp_hid"
                    EMBED_FILES "index.html")

# IDF v6.0 added fields to httpd_uri_t, mcpwm_*_config_t, ledc_*_config_t.
# Designated initializers that omit them are valid C++ (zero-init) but trigger
# -Wmissing-field-initializers which is promoted to error by the project flags.
if(NOT CMAKE_SCRIPT_MODE_FILE)
    add_dependencies(${COMPONENT_LIB} webapp_build)
endif()

set_source_files_properties("src/webServerHandler.cpp" "src/actuators.cpp"
    PROPERTIES COMPILE_FLAGS "-Wno-missing-field-initializers")
