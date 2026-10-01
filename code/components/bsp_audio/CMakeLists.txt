idf_component_register(
    SRCS "bsp_es8388.c" "bsp_i2s.c" "example/audio_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common bsp_i2c driver
)
