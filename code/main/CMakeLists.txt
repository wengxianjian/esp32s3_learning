idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES bsp_led bsp_key bsp_uart bsp_timer bsp_pwm bsp_adc bsp_rtc_rng bsp_i2c bsp_spi bsp_1wire bsp_infrared bsp_sdcard bsp_audio bsp_usb bsp_wifi bsp_net bsp_mqtt
)
