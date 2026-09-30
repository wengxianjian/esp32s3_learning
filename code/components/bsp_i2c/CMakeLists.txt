idf_component_register(
    SRCS "bsp_i2c.c" "bsp_xl9555.c" "bsp_eeprom.c" "bsp_oled.c" "bsp_ap3216c.c" "bsp_qma6100p.c"
         "example/xl9555_demo.c" "example/eeprom_demo.c" "example/oled_demo.c"
         "example/ap3216c_demo.c" "example/qma6100p_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common driver
)
