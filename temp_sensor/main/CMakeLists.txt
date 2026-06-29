idf_component_register(
    SRCS "temp_sensor_main.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp-idf-lib__dht espressif__ssd1306
)
