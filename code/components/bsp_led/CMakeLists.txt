idf_component_register(
    SRCS "bsp_led.c" "example/led_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common driver
)
