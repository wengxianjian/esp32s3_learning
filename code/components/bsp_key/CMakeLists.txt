idf_component_register(
    SRCS "bsp_key.c" "example/key_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common driver bsp_led
)
