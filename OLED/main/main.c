#include <stdio.h>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"

#define I2C_MASTER_SDA_IO GPIO_NUM_10
#define I2C_MASTER_SCL_IO GPIO_NUM_11

static i2c_master_dev_handle_t i2c_dev_handle = NULL;

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64

static uint8_t framebuffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
static uint8_t tx_buffer[1 + sizeof(framebuffer)];

typedef enum
{
    /* Fundamental commands */
    SSD1306_SET_CONTRAST = 0x81,
    SSD1306_DISPLAY_ALL_ON_RESUME = 0xA4,
    SSD1306_DISPLAY_ALL_ON = 0xA5,
    SSD1306_NORMAL_DISPLAY = 0xA6,
    SSD1306_INVERT_DISPLAY = 0xA7,
    SSD1306_DISPLAY_OFF = 0xAE,
    SSD1306_DISPLAY_ON = 0xAF,

    /* Addressing commands */
    SSD1306_SET_LOWER_COLUMN = 0x00,
    SSD1306_SET_HIGHER_COLUMN = 0x10,
    SSD1306_SET_MEMORY_ADDR_MODE = 0x20,
    SSD1306_SET_COLUMN_ADDR = 0x21,
    SSD1306_SET_PAGE_ADDR = 0x22,

    /* Hardware configuration */
    SSD1306_SET_START_LINE = 0x40,
    SSD1306_SET_SEGMENT_REMAP_NORMAL = 0xA0,
    SSD1306_SET_SEGMENT_REMAP_REVERSE = 0xA1,
    SSD1306_SET_MULTIPLEX = 0xA8,
    SSD1306_SET_COM_SCAN_INC = 0xC0,
    SSD1306_SET_COM_SCAN_DEC = 0xC8,
    SSD1306_SET_DISPLAY_OFFSET = 0xD3,
    SSD1306_SET_COM_PINS = 0xDA,

    /* Timing & driving */
    SSD1306_SET_DISPLAY_CLOCK_DIV = 0xD5,
    SSD1306_SET_PRECHARGE = 0xD9,
    SSD1306_SET_VCOM_DESELECT = 0xDB,
    SSD1306_NOP = 0xE3,

    /* Charge pump */
    SSD1306_SET_CHARGE_PUMP = 0x8D,

    /* Scrolling commands */
    SSD1306_RIGHT_HORIZONTAL_SCROLL = 0x26,
    SSD1306_LEFT_HORIZONTAL_SCROLL = 0x27,
    SSD1306_VERTICAL_RIGHT_SCROLL = 0x29,
    SSD1306_VERTICAL_LEFT_SCROLL = 0x2A,
    SSD1306_DEACTIVATE_SCROLL = 0x2E,
    SSD1306_ACTIVATE_SCROLL = 0x2F,
    SSD1306_SET_VERTICAL_SCROLL_AREA = 0xA3,

} ssd1306_command_t;

static const uint8_t ssd1306_init_sequence[] = {
    SSD1306_DISPLAY_OFF,              // Display OFF
    SSD1306_SET_DISPLAY_CLOCK_DIV,    // Set Display Clock Divide Ratio/Oscillator Frequency
    0x80,                             // clock divide ratio
    SSD1306_SET_MULTIPLEX,            // Set Multiplex Ratio
    0x3F,                             // Multiplex Ratio for 128x64 (64-1)
    SSD1306_SET_DISPLAY_OFFSET,       // Set Display Offset
    0x00,                             // Display Offset
    SSD1306_SET_START_LINE,           // Set Start Line
    SSD1306_SET_CHARGE_PUMP,          // Set Charge Pump
    0x14,                             // Enable Charge Pump
    SSD1306_SET_MEMORY_ADDR_MODE,     // Set Memory Addressing Mode
    0x00,                             // Horizontal Addressing Mode
    SSD1306_SET_SEGMENT_REMAP_NORMAL, // Set Segment Re-map
    SSD1306_SET_COM_SCAN_DEC,         // Set COM Output Scan Direction
    SSD1306_SET_COM_PINS,             // Set COM Pins Hardware Configuration
    0x12,                             // COM Pins Hardware Configuration
    SSD1306_SET_CONTRAST,             // Set Contrast Control
    0xFF,                             // Contrast Control
    SSD1306_SET_PRECHARGE,            // Set Pre-charge Period
    0xF1,                             // Pre-charge Period¨
    SSD1306_SET_VCOM_DESELECT,        // Set VCOMH Deselect Level
    0x40,                             // VCOMH Deselect Level
    SSD1306_DISPLAY_ALL_ON_RESUME,    // Entire Display ON
    SSD1306_NORMAL_DISPLAY,           // Set Normal Display
    SSD1306_DISPLAY_ON                // Display ON
};

esp_err_t ssd1306_write_command(uint8_t cmd)
{
    tx_buffer[0] = 0x00;
    tx_buffer[1] = cmd;
    return i2c_master_transmit(i2c_dev_handle, tx_buffer, 2, 1000);
}

esp_err_t ssd1306_write_commands(const uint8_t *cmds, size_t len)
{
    tx_buffer[0] = 0x00;
    memcpy(&tx_buffer[1], cmds, len);
    return i2c_master_transmit(i2c_dev_handle, tx_buffer, len + 1, 1000);
}

esp_err_t ssd1306_write_data(const uint8_t *data, size_t len)
{
    tx_buffer[0] = 0x40;
    memcpy(&tx_buffer[1], data, len);
    return i2c_master_transmit(i2c_dev_handle, tx_buffer, len + 1, 1000);
}

esp_err_t ssd1306_init(void)
{
    return ssd1306_write_commands(ssd1306_init_sequence, sizeof(ssd1306_init_sequence));
}

void ssd1306_clear_display(void)
{
    memset(framebuffer, 0, sizeof(framebuffer));
}

void ssd1306_fill_display(void)
{
    memset(framebuffer, 0xFF, sizeof(framebuffer));
}

esp_err_t ssd1306_draw_pixel(uint8_t x, uint8_t y, bool color)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
        return ESP_OK;
    uint8_t page = y / 8;
    uint16_t index = page * SSD1306_WIDTH + x;
    uint8_t bit = y % 8;
    if (color)
    {
        framebuffer[index] |= (1 << bit); // Set pixel
    }
    else
    {
        framebuffer[index] &= ~(1 << bit); // Clear pixel
    }

    return ESP_OK;
}

esp_err_t ssd1306_draw_hline(int x, int y, uint8_t length, bool color)
{
    if (y < 0 || y >= SSD1306_HEIGHT) return ESP_OK;
    if (length <= 0) return ESP_OK;

    
    int x_end = x + length;

    if (x_end < 0 || x >= SSD1306_WIDTH) return ESP_OK;

    if (x < 0)
    {
        length += x;   // reduce
        x = 0;
    }

    if (x + length > SSD1306_WIDTH)
    {
        length = SSD1306_WIDTH - x;
    }

    for (int i = 0; i < length; i++)
    {
        ssd1306_draw_pixel(x + i, y, color);
    }

    return ESP_OK;
}
esp_err_t ssd1306_draw_vline(int x, int y, uint8_t length, bool color)
{
    if (x < 0 || x >= SSD1306_WIDTH) return ESP_OK;
    if (y < 0 || y + length > SSD1306_HEIGHT) return ESP_OK;
    for (uint8_t i = 0; i < length; i++)
    {
        ESP_RETURN_ON_ERROR(ssd1306_draw_pixel(x, y + i, color), "ssd1306_draw_vline", "");
    }
    return ESP_OK;
}
esp_err_t ssd1306_draw_rectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool color)
{
    if (x + width > SSD1306_WIDTH || y + height > SSD1306_HEIGHT)
        return ESP_ERR_INVALID_ARG;

    ESP_RETURN_ON_ERROR(ssd1306_draw_hline(x, y, width, color), "ssd1306_draw_rectangle", "");
    ESP_RETURN_ON_ERROR(ssd1306_draw_hline(x, y + height - 1, width, color), "ssd1306_draw_rectangle", "");
    ESP_RETURN_ON_ERROR(ssd1306_draw_vline(x, y, height, color), "ssd1306_draw_rectangle", "");
    ESP_RETURN_ON_ERROR(ssd1306_draw_vline(x + width - 1, y, height, color), "ssd1306_draw_rectangle", "");

    return ESP_OK;
}
esp_err_t ssd1306_fill_rectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool color)
{
    if (x + width > SSD1306_WIDTH || y + height > SSD1306_HEIGHT)
        return ESP_ERR_INVALID_ARG;

    for (uint8_t i = 0; i < height; i++)
    {
        ESP_RETURN_ON_ERROR(ssd1306_draw_hline(x, y + i, width, color), "ssd1306_fill_rectangle", "");
    }

    return ESP_OK;
}
esp_err_t ssd1306_draw_circle(int cx, int cy, uint8_t r)
{
    if (r == 0)
        return ESP_ERR_INVALID_ARG;

    int x = 0;
    int y = r;
    int p = 1 - r;
    while (x <= y)
    {
        
        ssd1306_draw_pixel(cx + x, cy + y, true);
        ssd1306_draw_pixel(cx + x, cy - y, true);
        ssd1306_draw_pixel(cx - x, cy + y, true);
        ssd1306_draw_pixel(cx - x, cy - y, true);
        ssd1306_draw_pixel(cx + y, cy + x, true);
        ssd1306_draw_pixel(cx + y, cy - x, true);
        ssd1306_draw_pixel(cx - y, cy + x, true);
        ssd1306_draw_pixel(cx - y, cy - x, true);


        if (p < 0)
        {
            p += 2 * x + 3;
        }
        else
        {
            y--;
            p += 2 * (x - y) + 5;
        }
        x++;
    }
    return ESP_OK;
}
esp_err_t ssd1306_fill_circle(int cx, int cy, uint8_t r)
{
    if (r == 0)
        return ESP_ERR_INVALID_ARG;

    int x = 0;
    int y = r;
    int p = 1 - r;
    while (x <= y)
    {
        ssd1306_draw_hline(cx - x, cy + y, 2 * x + 1, true);
        ssd1306_draw_hline(cx - x, cy - y, 2 * x + 1, true);
        ssd1306_draw_hline(cx - y, cy + x, 2 * y + 1, true);
        ssd1306_draw_hline(cx - y, cy - x, 2 * y + 1, true);

        if (p < 0)
        {
            p += 2 * x + 3;
        }
        else
        {
            y--;
            p += 2 * (x - y) + 5;
        }
        x++;
    }
    return ESP_OK;
}

esp_err_t ssd1306_update(void)
{
    ESP_RETURN_ON_ERROR(ssd1306_write_command(SSD1306_SET_COLUMN_ADDR), "ssd1306_update", "");
    ESP_RETURN_ON_ERROR(ssd1306_write_command(0), "ssd1306_update", "");
    ESP_RETURN_ON_ERROR(ssd1306_write_command(SSD1306_WIDTH - 1), "ssd1306_update", "");

    ESP_RETURN_ON_ERROR(ssd1306_write_command(SSD1306_SET_PAGE_ADDR), "ssd1306_update", "");
    ESP_RETURN_ON_ERROR(ssd1306_write_command(0), "ssd1306_update", "");
    ESP_RETURN_ON_ERROR(ssd1306_write_command(SSD1306_HEIGHT / 8 - 1), "ssd1306_update", "");

    return ssd1306_write_data(framebuffer, sizeof(framebuffer));
}
esp_err_t ssd1306_set_contrast(uint8_t contrast)
{
    return ssd1306_write_commands((const uint8_t[]){SSD1306_SET_CONTRAST, contrast}, 2);
}

void app_main(void)
{
    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
    };

    i2c_master_bus_handle_t i2c_bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle));

    i2c_device_config_t i2c_dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x3C,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(
        i2c_bus_handle,
        &i2c_dev_config,
        &i2c_dev_handle));

    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(ssd1306_init());

    ssd1306_clear_display();
    ESP_ERROR_CHECK(ssd1306_update());

    while (true)
    {
        for (int r = 0; r < 5; r++)
        {
            ssd1306_clear_display();

            for (int R = 0; R < 20; R++)
            {
                ssd1306_draw_circle(64, 32, r + R * 5);
            }

            ESP_ERROR_CHECK(ssd1306_update());
            vTaskDelay(pdMS_TO_TICKS(30));
        }
    }
}
