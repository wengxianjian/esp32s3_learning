idf_component_register(
    SRCS "bsp_i2c.c" "bsp_xl9555.c" "bsp_eeprom.c" "bsp_oled.c"
         "example/xl9555_demo.c" "example/eeprom_demo.c" "example/oled_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common driver
)
