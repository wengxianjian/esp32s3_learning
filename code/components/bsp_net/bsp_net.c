#include "bsp_net.h"
#include "bsp_wifi.h"

esp_err_t bsp_net_wifi_connect(void)
{
    return bsp_wifi_sta_connect();
}
