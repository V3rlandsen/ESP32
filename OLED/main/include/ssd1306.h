#ifndef SSD1306_H
#define SSD1306_H


#include "driver/i2c_master.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>


typedef struct {
    i2c_master_dev_handle_t i2c_dev_handle;
    size_t width;
    size_t height;
} ssd1306_config_t;

typedef struct {
    i2c_master_dev_handle_t i2c_dev_handle;

    size_t width;
    size_t height;

    uint8_t *framebuffer;
    uint8_t *tx_buffer;
    size_t framebuffer_size;
} ssd1306_handle_t;



#define SSD1306_DEFAULT_WIDTH 128
#define SSD1306_DEFAULT_HEIGHT 64

//esp_err_t ssd1306_write_command(ssd1306_handle_t *handle, uint8_t cmd);
//esp_err_t ssd1306_write_commands(ssd1306_handle_t *handle, const uint8_t *cmds, size_t len);
//esp_err_t ssd1306_write_data(ssd1306_handle_t *handle, const uint8_t *data, size_t len);

esp_err_t ssd1306_init(ssd1306_handle_t *handle, const ssd1306_config_t *config);

void ssd1306_clear_display(ssd1306_handle_t *handle);
void ssd1306_fill_display(ssd1306_handle_t *handle);
void ssd1306_draw_pixel(ssd1306_handle_t *handle, uint8_t x, uint8_t y, bool color);
esp_err_t ssd1306_draw_hline(ssd1306_handle_t *handle, int x, int y, uint8_t length, bool color);
esp_err_t ssd1306_draw_vline(ssd1306_handle_t *handle, int x, int y, uint8_t length, bool color);
esp_err_t ssd1306_draw_line(ssd1306_handle_t *handle, int x0, int y0, int x1, int y1, bool color);
esp_err_t ssd1306_draw_rectangle(ssd1306_handle_t *handle, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool color);
esp_err_t ssd1306_fill_rectangle(ssd1306_handle_t *handle, uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool color);
esp_err_t ssd1306_draw_circle(ssd1306_handle_t *handle, int cx, int cy, uint8_t r);
esp_err_t ssd1306_fill_circle(ssd1306_handle_t *handle, int cx, int cy, uint8_t r);
esp_err_t ssd1306_draw_char(ssd1306_handle_t *handle, uint8_t x, uint8_t y, char c, bool color);
esp_err_t ssd1306_draw_string(ssd1306_handle_t *handle, uint8_t x, uint8_t y, const char *str, bool color);
//esp_err_t ssd1306_draw_bitmap_full_screen(ssd1306_handle_t *handle, const uint8_t *bitmap);
esp_err_t ssd1306_draw_bitmap(ssd1306_handle_t *handle, uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bitmap);
esp_err_t ssd1306_update(ssd1306_handle_t *handle);
esp_err_t ssd1306_set_contrast(ssd1306_handle_t *handle, uint8_t contrast);
esp_err_t ssd1306_set_display_on(ssd1306_handle_t *handle, bool on);

esp_err_t ssd1306_set_scroll_mode(ssd1306_handle_t *handle, bool right, uint8_t start_page, uint8_t end_page, uint8_t scroll_speed);
esp_err_t ssd1306_set_scroll(ssd1306_handle_t *handle, bool scroll);

#endif


