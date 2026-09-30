idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES bsp_led bsp_key bsp_uart bsp_timer bsp_pwm bsp_adc bsp_rtc_rng
)
