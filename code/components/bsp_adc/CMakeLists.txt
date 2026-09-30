idf_component_register(
    SRCS "bsp_adc.c" "example/adc_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common esp_adc
)
