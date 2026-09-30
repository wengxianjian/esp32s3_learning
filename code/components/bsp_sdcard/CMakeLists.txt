idf_component_register(
    SRCS "bsp_sdcard.c" "example/sdcard_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common driver sdmmc fatfs vfs
)
