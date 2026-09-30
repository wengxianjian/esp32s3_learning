#include "bsp_rtc_rng.h"
#include "board.h"
#include <sys/time.h>
#include "esp_random.h"
#include "driver/temperature_sensor.h"
#include "esp_log.h"

static const char *TAG = "bsp_rtc_rng";

/* ==================== RTC ==================== */

esp_err_t bsp_rtc_set_time(int year, int mon, int mday, int hour, int min, int sec)
{
    struct tm dt = {0};
    dt.tm_year = year - 1900;
    dt.tm_mon = mon - 1;
    dt.tm_mday = mday;
    dt.tm_hour = hour;
    dt.tm_min = min;
    dt.tm_sec = sec;

    time_t t = mktime(&dt);
    struct timeval tv = { .tv_sec = t, .tv_usec = 0 };

    if (settimeofday(&tv, NULL) != 0) {
        ESP_LOGE(TAG, "settimeofday failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void bsp_rtc_get_time(struct tm *dt)
{
    time_t now = time(NULL);
    localtime_r(&now, dt);
}

/* ==================== RNG ==================== */

uint32_t bsp_rng_random(void)
{
    return esp_random();
}

int bsp_rng_range(int min, int max)
{
    return (int)(esp_random() % (max - min + 1)) + min;
}

/* ==================== 内部温度传感器 ==================== */

static temperature_sensor_handle_t s_tsens = NULL;

esp_err_t bsp_tsens_init(void)
{
    temperature_sensor_config_t cfg = {
        .range_min = BSP_TSENS_RANGE_MIN,
        .range_max = BSP_TSENS_RANGE_MAX,
        .clk_src = TEMPERATURE_SENSOR_CLK_SRC_DEFAULT,
    };

    esp_err_t ret = temperature_sensor_install(&cfg, &s_tsens);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "temperature_sensor_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "temperature sensor installed (range %d~%d C)",
             BSP_TSENS_RANGE_MIN, BSP_TSENS_RANGE_MAX);
    return ESP_OK;
}

float bsp_tsens_read_celsius(void)
{
    float temp = 0.0f;
    esp_err_t ret = temperature_sensor_enable(s_tsens);
    if (ret == ESP_OK) {
        ret = temperature_sensor_get_celsius(s_tsens, &temp);
        temperature_sensor_disable(s_tsens);
    }
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "temperature_sensor_get_celsius failed: %s", esp_err_to_name(ret));
    }
    return temp;
}
