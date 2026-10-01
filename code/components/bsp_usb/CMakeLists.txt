idf_component_register(
    SRCS "bsp_usb.c" "example/usb_msc_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common esp_tinyusb esp_partition fatfs vfs wear_levelling
)
