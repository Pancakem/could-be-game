#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include "ntp.h"

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM

void sntp_sync_time(struct timeval *tv) {
  settimeofday(tv, NULL);
  ESP_LOGI(SNTP_CODE, "Time synchronized from custom code");
  sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}

#endif

void time_sync_notification_cb(struct timeval *tv) {
  ESP_LOGI(SNTP_CODE, "A time sync event!");
}

void initialize_sntp(void)
{
    ESP_LOGI(SNTP_CODE, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "2.ke.pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();
}

void obtain_time(void)
{
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(SNTP_CODE, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}
