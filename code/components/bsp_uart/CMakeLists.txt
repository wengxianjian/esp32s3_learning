idf_component_register(
    SRCS "bsp_uart.c" "example/uart_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common driver bsp_led
)
