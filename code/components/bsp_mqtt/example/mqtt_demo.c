/**
 * @file mqtt_demo.c
 * @brief MQTT 实验示例（对应指南第五十五章）
 *
 * ── 本示例做什么 ─────────────────────────────────────────────
 * 1) 调用 bsp_mqtt_start()：连 WiFi 并启动 MQTT 客户端（订阅 + 首发一条 hello）；
 * 2) 进入死循环，每 5 秒向 CONFIG_MQTT_TOPIC 发布一次 "periodic data"；
 * 3) 发布的确认与收到的消息在 bsp_mqtt.c 的事件回调里打印。
 *
 * ── 运行方式 ─────────────────────────────────────────────────
 * 在 menuconfig 中选择 "MQTT (publish/subscribe)"（对应宏 CONFIG_DEMO_MQTT），
 * Broker 与主题走 "MQTT Config" 的 "MQTT broker URI"（缺省 mqtt://test.mosquitto.org）
 * 与 "MQTT topic"（缺省 esp32s3/test）；app_main() 调用 bsp_mqtt_demo()。
 * 也可用 PC 端 MQTT 客户端订阅同一主题，观察开发板周期发布的数据。
 */

#include "bsp_mqtt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 日志标签：本示例日志显示为 "mqtt_demo" */
static const char *TAG = "mqtt_demo";

void bsp_mqtt_demo(void)
{
    ESP_LOGI(TAG, "MQTT demo start");
    /* 连 WiFi 并启动 MQTT 客户端；内部会订阅主题并首发一条 hello（失败断言复位） */
    ESP_ERROR_CHECK(bsp_mqtt_start());

    while (1) {
        /* 每 5 秒发一次数据（1000 tick/秒，故 5000） */
        vTaskDelay(pdMS_TO_TICKS(5000));
        /* 向 CONFIG_MQTT_TOPIC 发布 "periodic data"：
         * 参数依次为 客户端句柄、主题、负载、长度(0=自动计算)、QoS=0、retain=0 */
        esp_mqtt_client_publish(bsp_mqtt_get_client(), CONFIG_MQTT_TOPIC,
                                "periodic data", 0, 0, 0);
    }
}
