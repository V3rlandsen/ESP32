#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "driver/rmt_tx.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "driver/pulse_cnt.h"
#include "esp_rom_gpio.h"
#include "hal/gpio_types.h"
#include "esp_rom_gpio.h"

#define LED_GPIO 48
#define LED_NUM 1

#define PWM_GPIO 4
#define pwm_freq_hz 25000
#define FADE_TIME_MS 30000

#define TACHOMETER_PCNT_GPIO 5
#define TACHOMETER_PCNT_HIGH_LIMIT 32767
#define TACHOMETER_PCNT_LOW_LIMIT  -32768
#define TACHOMETER_PULSES_PER_REVOLUTION 2

#define ROTARY_ENCODER_GPIO_A 6
#define ROTARY_ENCODER_GPIO_B 7
#define ROTARY_ENCODER_PCNT_HIGH_LIMIT 1023
#define ROTARY_ENCODER_PCNT_LOW_LIMIT  -1024
#define ROTARY_ENCODER_PULSES_PER_REVOLUTION 4


static void rpm_task(void *arg)
{
    pcnt_unit_handle_t pcnt = (pcnt_unit_handle_t)arg;

    int last_count = 0;
    int count;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));

        pcnt_unit_get_count(pcnt, &count);

        int pulses = count - last_count;
        last_count = count;

        float rpm = (float)pulses * 60.0f / (float)TACHOMETER_PULSES_PER_REVOLUTION;
        ESP_LOGI("RPM", "RPM: %.2f", rpm);
    }
}

static inline uint32_t percent_to_duty(int percent)
{
    return (percent * 1023) / 100;
}


void app_main(void)
{
    // Initialize LED strip
    led_strip_handle_t led_strip;

    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_NUM,
        .led_model = LED_MODEL_WS2812,
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));


    // Initialize pwm
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = pwm_freq_hz,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));


    ledc_channel_config_t channel_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PWM_GPIO,
        .duty = 0,      
        .hpoint = 0,
    };

    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));
    ESP_ERROR_CHECK(ledc_fade_func_install(0)); 

    // Initialize pulse counter (tachometer)
    pcnt_unit_config_t tachometer_pcnt_unit_config = {
        .high_limit = TACHOMETER_PCNT_HIGH_LIMIT,
        .low_limit = TACHOMETER_PCNT_LOW_LIMIT,
        .flags.accum_count = 1,
    };

    pcnt_unit_handle_t tachometer_pcnt_unit;

    ESP_ERROR_CHECK(pcnt_new_unit(&tachometer_pcnt_unit_config, &tachometer_pcnt_unit));

    pcnt_chan_config_t tachometer_pcnt_channel_config = {
        .edge_gpio_num = TACHOMETER_PCNT_GPIO,
    };

    pcnt_channel_handle_t tachometer_pcnt_channel = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(tachometer_pcnt_unit, &tachometer_pcnt_channel_config, &tachometer_pcnt_channel));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(tachometer_pcnt_channel, PCNT_CHANNEL_EDGE_ACTION_INCREASE,PCNT_CHANNEL_EDGE_ACTION_HOLD));
    ESP_ERROR_CHECK(pcnt_unit_enable(tachometer_pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(tachometer_pcnt_unit));

    xTaskCreate(rpm_task, "rpm_task", 4096, tachometer_pcnt_unit, 5, NULL);

    // Initialize rotary encoder

    // 1. Create unit
    pcnt_unit_config_t rotary_encoder_pcnt_unit_config = {
        .high_limit = ROTARY_ENCODER_PCNT_HIGH_LIMIT,
        .low_limit = ROTARY_ENCODER_PCNT_LOW_LIMIT,
    };
    pcnt_unit_handle_t rotary_encoder_pcnt_unit;
    ESP_ERROR_CHECK(pcnt_new_unit(&rotary_encoder_pcnt_unit_config, &rotary_encoder_pcnt_unit));

    // 2. Glitch filter (must be before enable)
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 10000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(rotary_encoder_pcnt_unit, &filter_config));

    // 3. Create channel and set actions
    pcnt_chan_config_t rotary_encoder_pcnt_channel_config = {
        .edge_gpio_num = ROTARY_ENCODER_GPIO_A,
        .level_gpio_num = ROTARY_ENCODER_GPIO_B,
    };
    pcnt_channel_handle_t rotary_encoder_pcnt_channel = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(rotary_encoder_pcnt_unit, &rotary_encoder_pcnt_channel_config, &rotary_encoder_pcnt_channel));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(rotary_encoder_pcnt_channel,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(rotary_encoder_pcnt_channel,
        PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    // 4. Enable and start
    ESP_ERROR_CHECK(pcnt_unit_enable(rotary_encoder_pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(rotary_encoder_pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(rotary_encoder_pcnt_unit));



    int last_rotary_count = 0;
    int duty_percent = 0;
    uint8_t r = 0, g = 0, b = 0;
    while (1) {
        int rotary_count;
        ESP_ERROR_CHECK(pcnt_unit_get_count(rotary_encoder_pcnt_unit, &rotary_count));
        

        
        int delta = rotary_count - last_rotary_count;
        if (delta != 0) {

            if (delta > 0) {
                duty_percent += 10;
                
            } else {
                duty_percent -= 10;
               
            }

            if (duty_percent > 100) duty_percent = 100;
            if (duty_percent < 0) duty_percent = 0;
            ESP_LOGI("Main", "Fan duty cycle: %d%%", duty_percent);

            last_rotary_count = rotary_count;
        }
        vTaskDelay(pdMS_TO_TICKS(10));

        uint32_t duty = percent_to_duty(duty_percent);

        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));

        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

        if (duty_percent == 0) {
            r = 0; g = 0; b = 10;   // svak blå (idle)
        }
        else if (duty_percent < 50) {
            r = 0;
            g = 50;
            b = 0;   // grønn
        }
        else if (duty_percent < 80) {
            r = 50;
            g = 30;
            b = 0;   // gul/orange
        }
        else {
            r = 50;
            g = 0;
            b = 0;   // rød
        }
        led_strip_set_pixel(led_strip, 0, r, g, b);
        led_strip_refresh(led_strip);
    }
}

