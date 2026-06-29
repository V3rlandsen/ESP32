#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"  // Klassisk I2C-driver
#include "ssd1306.h"     // Din installerte espressif/ssd1306-komponent

static const char *TAG = "OLED_HELLO_WORLD";

#define I2C_MASTER_NUM        I2C_NUM_0
#define PIN_NUM_SCL           4
#define PIN_NUM_SDA           5

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Starter I2C og rydder skjerm...");

    // 1. Initialiser I2C-bussen
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = (gpio_num_t)PIN_NUM_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = (gpio_num_t)PIN_NUM_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000, 
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));

    // 2. Opprett kobling til SSD1306-skjermen
    ssd1306_handle_t ssd1306_dev = ssd1306_create(I2C_MASTER_NUM, 0x3C);
    if (ssd1306_dev == NULL) {
        ESP_LOGE(TAG, "Kunne ikke opprette SSD1306-skjermen!");
        return;
    }

    // 3. Tøm skjermen helt (slå av den skrå linjen og gjør den svart)
    ssd1306_clear_screen(ssd1306_dev, 0x00);
    ssd1306_refresh_gram(ssd1306_dev);

    ESP_LOGI(TAG, "Skriver HELLO WORLD til skjerm-minnet...");

    // 4. Tegn tekst
    // Parametere: (enhet, X-posisjon, Y-posisjon, tekst, fontstørrelse 16, modus 1=hvit skrift)
    ssd1306_draw_string(ssd1306_dev, 0, 0, (const uint8_t *)"HELLO", 16, 1);

    // 5. Skyv teksten fra minnet ut til glasset på skjermen
    ssd1306_refresh_gram(ssd1306_dev);
    ESP_LOGI(TAG, "Tekst sendt til skjermen!");

    // Loop for å holde programmet i gang
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
