idf_component_register(
    SRCS "bsp_lcd.c" "example/lcd_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common bsp_i2c driver
)
