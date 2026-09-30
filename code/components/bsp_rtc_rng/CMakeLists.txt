idf_component_register(
    SRCS "bsp_rtc_rng.c" "example/rtc_demo.c" "example/rng_demo.c" "example/tsens_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common esp_driver_tsens
)
