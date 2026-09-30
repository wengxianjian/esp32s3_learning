idf_component_register(
    SRCS "bsp_pwm.c" "example/swpwm_demo.c" "example/hwpwm_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common driver
)
