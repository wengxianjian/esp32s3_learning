idf_component_register(
    SRCS "bsp_ds18b20.c" "example/ds18b20_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common driver
)
