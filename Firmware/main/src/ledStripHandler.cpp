#include "ledStripHandler.h"
#include "sdkconfig.h"
#include "pins.h"
#include <vector>
#include <string.h>

#define LED_STRIP_MEMORY_BLOCK_WORDS 0
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)
#define MAX_LEDS_SUPPORTED 256

// Estructura para nuestro búfer de software
typedef struct {
    uint8_t r, g, b;
} rgb_t;

led_strip_handle_t led_strip;
static rgb_t led_buffer[MAX_LEDS_SUPPORTED]; // Nuestro "lienzo" o búfer de software

// Función auxiliar para parsear el string de LEDs y aplicar colores AL BÚFER
static void parse_and_set_leds_to_buffer(const char* led_str, uint8_t r, uint8_t g, uint8_t b, uint8_t brillo);

void setupLedStrip(VehicleState* state) {
    led_strip_config_t strip_config = {
        .strip_gpio_num = (gpio_num_t)LED_STRIP_GPIO_PIN,
        .max_leds = MAX_LEDS_SUPPORTED,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = { .invert_out = false },
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = LED_STRIP_RMT_RES_HZ,
        .mem_block_symbols = LED_STRIP_MEMORY_BLOCK_WORDS,
        .flags = { .with_dma = 0 },
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip); // Limpiamos la tira una sola vez al inicio
}

void handleLedStrip(VehicleState* state) {
    if (!state) return;

    // 1. Limpiar nuestro búfer de software, no la tira física
    memset(led_buffer, 0, sizeof(led_buffer));

    static unsigned long lastBlinkTime = 0;
    static bool blinkState = false;
    if (millis() - lastBlinkTime > 500) {
        lastBlinkTime = millis();
        blinkState = !blinkState;
    }

    // 2. "Pintar" las capas de colores en el búfer de software
    if (state->luces > 0) {
        for (const auto& group : state->ledGroups) {
            uint8_t final_brillo = group.brillo;
            if (group.funcion == LUZ_POSICION_TRASERA || group.funcion == LUZ_POSICION_DELANTERA) {
                switch(state->luces) {
                    case 1: final_brillo = (uint8_t)((float)group.brillo * 0.2f); break;
                    case 2: final_brillo = (uint8_t)((float)group.brillo * 0.6f); break;
                    case 3: final_brillo = group.brillo; break;
                    default: final_brillo = 0;
                }
                if (final_brillo > 0) {
                    parse_and_set_leds_to_buffer(group.leds, group.colorR, group.colorG, group.colorB, final_brillo);
                }
            } else if (group.funcion == NEON_INFERIOR || group.funcion == ILUMINACION_INTERIOR) {
                parse_and_set_leds_to_buffer(group.leds, group.colorR, group.colorG, group.colorB, group.brillo);
            }
        }
    }

    if (state->brakesLedOn) {
        for (const auto& group : state->ledGroups) {
            if ((group.funcion == LUZ_FRENO) || (group.funcion == LUZ_REVERSA)){
                parse_and_set_leds_to_buffer(group.leds, group.colorR, group.colorG, group.colorB, group.brillo);
            }
        }
    }
    if (state->reverseLedOn) {
        for (const auto& group : state->ledGroups) {
            if ((group.funcion == LUZ_FRENO) || (group.funcion == LUZ_REVERSA)){
                parse_and_set_leds_to_buffer(group.leds, group.colorR, group.colorG, group.colorB, group.brillo);
            }
        }
    }

    bool leftSignal = state->giroIzquierdo || state->baliza;
    bool rightSignal = state->giroDerecho || state->baliza;
    if (blinkState) {
        if (leftSignal) {
            for (const auto& group : state->ledGroups) {
                if (group.funcion == LUZ_GIRO_IZQUIERDA) {
                    parse_and_set_leds_to_buffer(group.leds, group.colorR, group.colorG, group.colorB, group.brillo);
                }
            }
        }
        if (rightSignal) {
            for (const auto& group : state->ledGroups) {
                if (group.funcion == LUZ_GIRO_DERECHA) {
                    parse_and_set_leds_to_buffer(group.leds, group.colorR, group.colorG, group.colorB, group.brillo);
                }
            }
        }
    }

    // 3. Copiar el búfer de software a la tira de LEDs física de una sola vez
    for (uint32_t i = 0; i < state->ledCount && i < MAX_LEDS_SUPPORTED; i++) {
        led_strip_set_pixel(led_strip, i, led_buffer[i].r, led_buffer[i].g, led_buffer[i].b);
    }

    // 4. Refrescar la tira para mostrar los cambios
    led_strip_refresh(led_strip);
}

static void parse_and_set_leds_to_buffer(const char* led_str, uint8_t r, uint8_t g, uint8_t b, uint8_t brillo) {
    char buffer[64];
    strncpy(buffer, led_str, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    uint8_t final_r = (uint8_t)((float)r * (float)brillo / 100.0f);
    uint8_t final_g = (uint8_t)((float)g * (float)brillo / 100.0f);
    uint8_t final_b = (uint8_t)((float)b * (float)brillo / 100.0f);

    char* part = strtok(buffer, ", ");
    while (part != NULL) {
        char* hyphen = strchr(part, '-');
        if (hyphen) {
            *hyphen = '\0';
            int start = atoi(part);
            int end = atoi(hyphen + 1);
            for (int i = start; i <= end; ++i) {
                if (i < MAX_LEDS_SUPPORTED) {
                    led_buffer[i] = {final_r, final_g, final_b};
                }
            }
        } else {
            int led_index = atoi(part);
            if (led_index < MAX_LEDS_SUPPORTED) {
                led_buffer[led_index] = {final_r, final_g, final_b};
            }
        }
        part = strtok(NULL, ", ");
    }
}