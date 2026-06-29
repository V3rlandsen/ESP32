#include "driver/ledc.h"
#include "esp_err.h"

#define FAN_PWM_PIN       (4)
#define FAN_LEDC_TIMER    LEDC_TIMER_0
#define FAN_LEDC_CHANNEL  LEDC_CHANNEL_0

void init_fan_pwm() {
    // 1. Configure the Shared Timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE, // ESP32-S3 only supports Low Speed Mode
        .timer_num        = FAN_LEDC_TIMER,
        .duty_resolution  = LEDC_TIMER_10_BIT,   // 0 to 1023 range
        .freq_hz          = 25000,               // 25 kHz Target Frequency
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 2. Configure the Independent Channel
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = FAN_LEDC_CHANNEL,
        .timer_sel      = FAN_LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = FAN_PWM_PIN,
        .duty           = 0, // Start completely turned off
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void set_fan_speed_percentage(uint32_t percentage) {
    if (percentage > 100) percentage = 100;
    
    // Convert percentage to 10-bit duty cycle target (0 to 1023)
    uint32_t duty_target = (percentage * 1023) / 100;
    
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, FAN_LEDC_CHANNEL, duty_target));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, FAN_LEDC_CHANNEL)); // Commit changes
}
