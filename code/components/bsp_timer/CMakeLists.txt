idf_component_register(
    SRCS "bsp_timer.c" "example/esptimer_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common esp_timer bsp_led
)
