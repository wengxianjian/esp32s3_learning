idf_component_register(
    SRCS "ir_nec_encoder.c" "bsp_ir_rx.c" "bsp_ir_tx.c"
         "example/ir_rx_demo.c" "example/ir_tx_demo.c"
    INCLUDE_DIRS "include"
    REQUIRES bsp_common driver
)
