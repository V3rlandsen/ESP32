#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"
#include "driver/gpio.h"

static const char *TAG = "RGB_TEST";

#define BLINK_GPIO 48     // Prøv også 47 hvis dette ikke virker
#define BUTTON_GPIO 4

static led_strip_handle_t led_strip;
static int current_color = 0;

static uint32_t last_debounce_time = 0;
#define DEBOUNCE_DELAY_MS 50

void app_main(void)
{
    ESP_LOGI(TAG, "Starter WS2812 test på GPIO %d", BLINK_GPIO);

    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,      // ← Dette
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ESP_LOGI(TAG, "Kjører knapp farge test");

    while (true) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (gpio_get_level(BUTTON_GPIO) == 0 && (current_time - last_debounce_time) > DEBOUNCE_DELAY_MS) {
            ESP_LOGI(TAG, "time: %u ms, button pressed", current_time);
            ESP_LOGI(TAG, "Debounce: %u ms since last press", current_time - last_debounce_time);
            
            last_debounce_time = current_time;

            current_color = (current_color + 1) % 4;
            switch (current_color) {
                case 0:  // Rød
                    led_strip_set_pixel(led_strip, 0, 255, 0, 0);
                    ESP_LOGI(TAG, "🔴 RØD");
                    break;
                case 1:  // Grønn
                    led_strip_set_pixel(led_strip, 0, 0, 255, 0);
                    ESP_LOGI(TAG, "🟢 GRØNN");
                    break;
                case 2:  // Blå
                    led_strip_set_pixel(led_strip, 0, 0, 0, 255);
                    ESP_LOGI(TAG, "🔵 BLÅ");
                    break;
                case 3:  // Hvit
                    led_strip_set_pixel(led_strip, 0, 120, 120, 120);
                    ESP_LOGI(TAG, "⚪ HVIT");
                    break;
            }
            led_strip_refresh(led_strip);
            while (gpio_get_level(BUTTON_GPIO) == 0) {
                vTaskDelay(pdMS_TO_TICKS(20));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}