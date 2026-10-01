#include "bsp_mqtt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "mqtt_demo";

void bsp_mqtt_demo(void)
{
    ESP_LOGI(TAG, "MQTT demo start");
    ESP_ERROR_CHECK(bsp_mqtt_start());

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_mqtt_client_publish(bsp_mqtt_get_client(), CONFIG_MQTT_TOPIC,
                                "periodic data", 0, 0, 0);
    }
}
