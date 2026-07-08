
#include "ssd1306.h"
#include <stdio.h>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "font.h"
#define SSD1306_CONTROL_COMMAND 0x00
#define SSD1306_CONTROL_DATA    0x40
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
    SSD1306_DISPLAY_OFF,               // Display OFF
    SSD1306_SET_DISPLAY_CLOCK_DIV,     // Set Display Clock Divide Ratio/Oscillator Frequency
    0x80,                              // clock divide ratio
    SSD1306_SET_MULTIPLEX,             // Set Multiplex Ratio
    0x3F,                              // Multiplex Ratio for 128x64 (64-1)
    SSD1306_SET_DISPLAY_OFFSET,        // Set Display Offset
    0x00,                              // Display Offset
    SSD1306_SET_START_LINE,            // Set Start Line
    SSD1306_SET_CHARGE_PUMP,           // Set Charge Pump
    0x14,                              // Enable Charge Pump
    SSD1306_SET_MEMORY_ADDR_MODE,      // Set Memory Addressing Mode
    0x00,                              // Horizontal Addressing Mode
    SSD1306_SET_SEGMENT_REMAP_REVERSE, // Set Segment Re-map
    SSD1306_SET_COM_SCAN_DEC,          // Set COM Output Scan Direction
    SSD1306_SET_COM_PINS,              // Set COM Pins Hardware Configuration
    0x12,                              // COM Pins Hardware Configuration
    SSD1306_SET_CONTRAST,              // Set Contrast Control
    0xFF,                              // Contrast Control
    SSD1306_SET_PRECHARGE,             // Set Pre-charge Period
    0xF1,                              // Pre-charge Period¨
    SSD1306_SET_VCOM_DESELECT,         // Set VCOMH Deselect Level
    0x40,                              // VCOMH Deselect Level
    SSD1306_DISPLAY_ALL_ON_RESUME,     // Entire Display ON
    SSD1306_NORMAL_DISPLAY,            // Set Normal Display
    SSD1306_DEACTIVATE_SCROLL,          // Deactivate Scroll
    SSD1306_DISPLAY_ON                 // Display ON
};

static esp_err_t ssd1306_write_command(ssd1306_handle_t *handle, uint8_t cmd)
{
    handle->tx_buffer[0] = 0x00;
    handle->tx_buffer[1] = cmd;
    return i2c_master_transmit(handle->i2c_dev_handle, handle->tx_buffer, 2, 1000);
}

static esp_err_t ssd1306_write_commands(ssd1306_handle_t *handle, const uint8_t *cmds, size_t len)
{
    handle->tx_buffer[0] = 0x00;
    memcpy(&handle->tx_buffer[1], cmds, len);
    return i2c_master_transmit(handle->i2c_dev_handle, handle->tx_buffer, len + 1, 1000);
}

static esp_err_t ssd1306_write_data(ssd1306_handle_t *handle, const uint8_t *data, size_t len)
{
    handle->tx_buffer[0] = 0x40;
    memcpy(&handle->tx_buffer[1], data, len);
    return i2c_master_transmit(handle->i2c_dev_handle, handle->tx_buffer, len + 1, 1000);
}

esp_err_t ssd1306_init(ssd1306_handle_t *handle,const ssd1306_config_t *config)
{
    handle->i2c_dev_handle = config->i2c_dev_handle;
    handle->width = config->width;
    handle->height = config->height;

    handle->framebuffer_size = config->width * config->height / 8;

    handle->framebuffer = calloc(handle->framebuffer_size, 1);
    handle->tx_buffer = malloc(handle->framebuffer_size + 1);

    if (!handle->framebuffer || !handle->tx_buffer) {
        free(handle->framebuffer);
        free(handle->tx_buffer);
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(ssd1306_write_commands(handle, ssd1306_init_sequence, sizeof(ssd1306_init_sequence)), "ssd1306_init", "");
    return ESP_OK;
}


void ssd1306_clear_display(ssd1306_handle_t *handle)
{
    memset(handle->framebuffer, 0, handle->framebuffer_size);
}

void ssd1306_fill_display(ssd1306_handle_t *handle)
{
    memset(handle->framebuffer, 0xFF, handle->framebuffer_size);
}

void ssd1306_draw_pixel(ssd1306_handle_t *handle, uint8_t x, uint8_t y, bool color)
{
    if (x >= handle->width || y >= handle->height)
        return;
    uint8_t page = y / 8;
    uint16_t index = page * handle->width + x;
    uint8_t bit = y % 8;
    if (color)
    {
        handle->framebuffer[index] |= (1 << bit); // Set pixel
    }
    else
    {
        handle->framebuffer[index] &= ~(1 << bit); // Clear pixel
    }

    return;
}

esp_err_t ssd1306_draw_hline(ssd1306_handle_t *handle, int x, int y, uint8_t length, bool color)
{
    if (y < 0 || y >= handle->height)
        return ESP_OK;
    if (length <= 0)
        return ESP_OK;

    int x_end = x + length;

    if (x_end < 0 || x >= handle->width)
        return ESP_OK;

    if (x < 0)
    {
        length += x; // reduce
        x = 0;
    }

    if (x + length > handle->width)
    {
        length = handle->width - x;
    }

    for (int i = 0; i < length; i++)
    {
        ssd1306_draw_pixel(handle, x + i, y, color);
    }

    return ESP_OK;
}
esp_err_t ssd1306_draw_vline(ssd1306_handle_t *handle, int x, int y, uint8_t length, bool color)
{
    if (x < 0 || x >= handle->width)
        return ESP_OK;
    if (y < 0 || y + length > handle->height)
        return ESP_OK;
    for (uint8_t i = 0; i < length; i++)
    {
        ssd1306_draw_pixel(handle, x, y + i, color);
    }
    return ESP_OK;
}
esp_err_t ssd1306_draw_line(ssd1306_handle_t *handle, int x0, int y0, int x1, int y1, bool color)
{
    int dx = abs(x1 - x0);
    int sx = (x0 < x1) ? 1 : -1;

    int dy = -abs(y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;

    int err = dx + dy;

    while (true)
    {
        ssd1306_draw_pixel(handle, x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }

    return ESP_OK;
}

esp_err_t ssd1306_draw_rectangle(ssd1306_handle_t *handle, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool color)
{
    if ((int)x + width > handle->width || (int)y + height > handle->height)
        return ESP_ERR_INVALID_ARG;

    ESP_RETURN_ON_ERROR(ssd1306_draw_hline(handle, x, y, width, color), "ssd1306_draw_rectangle", "");
    ESP_RETURN_ON_ERROR(ssd1306_draw_hline(handle, x, y + height - 1, width, color), "ssd1306_draw_rectangle", "");
    ESP_RETURN_ON_ERROR(ssd1306_draw_vline(handle, x, y, height, color), "ssd1306_draw_rectangle", "");
    ESP_RETURN_ON_ERROR(ssd1306_draw_vline(handle, x + width - 1, y, height, color), "ssd1306_draw_rectangle", "");

    return ESP_OK;
}
esp_err_t ssd1306_fill_rectangle(ssd1306_handle_t *handle, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool color)
{
    if ((int)x + width > handle->width || (int)y + height > handle->height)
        return ESP_ERR_INVALID_ARG;

    for (uint8_t i = 0; i < height; i++)
    {
        ESP_RETURN_ON_ERROR(ssd1306_draw_hline(handle, x, y + i, width, color), "ssd1306_fill_rectangle", "");
    }

    return ESP_OK;
}
esp_err_t ssd1306_draw_circle(ssd1306_handle_t *handle, int cx, int cy, uint8_t r)
{
    if (r == 0)
        return ESP_ERR_INVALID_ARG;

    int x = 0;
    int y = r;
    int p = 1 - r;
    while (x <= y)
    {

        ssd1306_draw_pixel(handle, cx + x, cy + y, true);
        ssd1306_draw_pixel(handle, cx + x, cy - y, true);
        ssd1306_draw_pixel(handle, cx - x, cy + y, true);
        ssd1306_draw_pixel(handle, cx - x, cy - y, true);
        ssd1306_draw_pixel(handle, cx + y, cy + x, true);
        ssd1306_draw_pixel(handle, cx + y, cy - x, true);
        ssd1306_draw_pixel(handle, cx - y, cy + x, true);
        ssd1306_draw_pixel(handle, cx - y, cy - x, true);

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
esp_err_t ssd1306_fill_circle(ssd1306_handle_t *handle, int cx, int cy, uint8_t r)
{
    if (r == 0)
        return ESP_ERR_INVALID_ARG;

    int x = 0;
    int y = r;
    int p = 1 - r;
    while (x <= y)
    {
        ESP_RETURN_ON_ERROR(ssd1306_draw_hline(handle, cx - x, cy + y, 2 * x + 1, true), "ssd1306_fill_circle", "");
        ESP_RETURN_ON_ERROR(ssd1306_draw_hline(handle, cx - x, cy - y, 2 * x + 1, true), "ssd1306_fill_circle", "");
        ESP_RETURN_ON_ERROR(ssd1306_draw_hline(handle, cx - y, cy + x, 2 * y + 1, true), "ssd1306_fill_circle", "");
        ESP_RETURN_ON_ERROR(ssd1306_draw_hline(handle, cx - y, cy - x, 2 * y + 1, true), "ssd1306_fill_circle", "");

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

esp_err_t ssd1306_draw_char(ssd1306_handle_t *handle, uint8_t x, uint8_t y, char c, bool color)
{
    if (c < FONT5X7_FIRST_CHAR || c > FONT5X7_LAST_CHAR)
        return ESP_OK;

    const uint8_t *glyph = font5x7[(uint8_t)c - FONT5X7_FIRST_CHAR];

    for (int col = 0; col < 5; col++)
    {
        uint8_t line = glyph[col];

        for (int row = 0; row < 7; row++)
        {
            if(x+col >= handle->width || y+row >= handle->height)
                continue;
            if ((line >> row) & 1)
            {
                ssd1306_draw_pixel(handle, x + col, y + row, color);
            }
        }
    }

    return ESP_OK;
}
esp_err_t ssd1306_draw_string(ssd1306_handle_t *handle, uint8_t x, uint8_t y, const char *str, bool color)
{
    while (*str)
    {
        if (x + 6 >= handle->width)
        {
            x = 0;
            y += 8;
        }
        if (y + 7 >= handle->height)
        {
            break;
        }
        ssd1306_draw_char(handle, x, y, *str, color);

        x += 6;
        str++;
    }

    return ESP_OK;
}

esp_err_t ssd1306_draw_bitmap(ssd1306_handle_t *handle, uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bitmap)
{

    for (int j = 0; j < height; j++)
    {
        uint8_t page = j / 8;
        uint8_t bit  = 1 << (j % 8);

        for (int i = 0; i < width; i++)
        {
            if (x + i >= handle->width || y + j >= handle->height)
                continue;
            uint8_t byte = bitmap[page * width + i];
            bool color = byte & bit;
            ssd1306_draw_pixel(handle, x + i, y + j, color);
        }
    }

    return ESP_OK;
}

esp_err_t ssd1306_update(ssd1306_handle_t *handle)
{
    ESP_RETURN_ON_ERROR(ssd1306_write_command(handle, SSD1306_SET_COLUMN_ADDR), "ssd1306_update", "");
    ESP_RETURN_ON_ERROR(ssd1306_write_command(handle, 0), "ssd1306_update", "");
    ESP_RETURN_ON_ERROR(ssd1306_write_command(handle, handle->width - 1), "ssd1306_update", "");

    ESP_RETURN_ON_ERROR(ssd1306_write_command(handle, SSD1306_SET_PAGE_ADDR), "ssd1306_update", "");
    ESP_RETURN_ON_ERROR(ssd1306_write_command(handle, 0), "ssd1306_update", "");
    ESP_RETURN_ON_ERROR(ssd1306_write_command(handle, handle->height / 8 - 1), "ssd1306_update", "");

    return ssd1306_write_data(handle, handle->framebuffer, handle->framebuffer_size);
}
esp_err_t ssd1306_set_contrast(ssd1306_handle_t *handle, uint8_t contrast)
{
    return ssd1306_write_commands(handle, (const uint8_t[]){SSD1306_SET_CONTRAST, contrast}, 2);
}
esp_err_t ssd1306_set_display_on(ssd1306_handle_t *handle, bool on){
    uint8_t cmd=on?SSD1306_DISPLAY_ON:SSD1306_DISPLAY_OFF;
    return ssd1306_write_command(handle, cmd);
}

esp_err_t ssd1306_set_scroll_mode(ssd1306_handle_t *handle, bool right, uint8_t start_page, uint8_t end_page, uint8_t scroll_speed)
{
    if (start_page > 7 || end_page > 7 || start_page > end_page)
        return ESP_ERR_INVALID_ARG;
    if(scroll_speed > 7)
        return ESP_ERR_INVALID_ARG;
    

    uint8_t cmd[7];
    cmd[0] = right ? SSD1306_RIGHT_HORIZONTAL_SCROLL : SSD1306_LEFT_HORIZONTAL_SCROLL;
    cmd[1] = 0x00; // Dummy byte
    cmd[2] = start_page;
    cmd[3] = scroll_speed;
    cmd[4] = end_page;
    cmd[5] = 0x00; // Dummy byte
    cmd[6] = 0xFF; // Dummy byte
    return ssd1306_write_commands(handle, cmd, 7);
}
esp_err_t ssd1306_set_scroll(ssd1306_handle_t *handle, bool scroll){
    return ssd1306_write_command(handle, scroll?SSD1306_ACTIVATE_SCROLL:SSD1306_DEACTIVATE_SCROLL);
}