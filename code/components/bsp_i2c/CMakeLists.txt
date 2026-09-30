idf_component_register(
    SRCS "bsp_i2c.c" "bsp_xl9555.c" "example/xl9555_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common driver
)
