#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "ntp.h"
#include "http_comm.h"
#include "connect.c"
#include "ssd_i2c.c"
#include "button.c"


#define tag "COULD BE GAME"

// number of times ESP32 has restarted since first boot
RTC_DATA_ATTR static int boot_count = 0;

void tag_handler(uint8_t* serial_no) {
  char *serial_str = (char *)malloc(15 * sizeof(char));
  char temp[8];
    for(int i = 0; i < 5; i++) {
      sprintf(temp, "%#x ", serial_no[i]);
      strcat(serial_str, temp);
    }

    ESP_LOGI(tag, "tag read: %s ", serial_str);

    // send serial over http
    BaseType_t xReturned;
    TaskHandle_t xHandle = NULL;
    xReturned = xTaskCreate(&https_get_task, "https_get_task", 8192, serial_str, 1, &xHandle);
    if (xReturned == pdPASS) {
      vTaskDelete(xHandle);
    }
    free(serial_str);
}

void app_main(void) {
  ESP_ERROR_CHECK( nvs_flash_init() );
  wifi_init_sta();
  ++boot_count;

  ESP_LOGI(SNTP_CODE, "Boot count: %d", boot_count);

  i2c_master_init();
  ssd1306_init();
  xTaskCreate(&task_ssd1306_display_clear, "ssd1306_display_clear",  2048, NULL, 6, NULL);

  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  // if time is not set tm_year == (1970 - 1900)
  if (timeinfo.tm_year < (2020 - 1900)) {
    ESP_LOGI(SNTP_CODE, "Time not set. Trying to sync to NTP time ...");
    obtain_time();
    // update time
    time(&now);
  }

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
  else {
    ESP_LOGI(SNTP_CODE, "Time was set. Adjusting ...");
     vTaskDelay(1000/portTICK_PERIOD_MS);
    xTaskCreate(&task_ssd1306_display_text, "ssd1306_display_text",  2048,
	     (void *)"Adjusting ...", 6, NULL);
     vTaskDelay(1000/portTICK_PERIOD_MS);
    obtain_time();
    time(&now);
    
  }
#endif

  char strftime_buf[64];
  // set timezone
  setenv("TZ", "EAT", 3);
  tzset();
  localtime_r(&now, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(SNTP_CODE, "The current datetime in Kenya is: %s", strftime_buf);

  xTaskCreate(&task_ssd1306_display_text, "ssd1306_display_text",  2048,
	      (void *)"Current time is:\n", 6, NULL);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  xTaskCreate(&task_ssd1306_display_text, "ssd1306_display_text",  2048,
	     strftime_buf, 6, NULL);

  ESP_LOGI("test", "================================\nTesting HTTP");
  BaseType_t xReturned;
  TaskHandle_t xHandle = NULL;
  xReturned = xTaskCreate(&https_get_task, "https_get_task", 8192, "Hello ... :[] ..", 1, &xHandle);
  if (xReturned == pdPASS) {
      vTaskDelete(xHandle);
  }
  ESP_LOGI(SNTP_CODE, "Test Completed\n============================");

 
 vTaskDelay(1000/portTICK_PERIOD_MS);
 
 xTaskCreate(&task_ssd1306_contrast, "ssid1306_contrast", 2048, NULL, 6, NULL);
 xTaskCreate(&task_ssd1306_scroll, "ssid1306_scroll", 2048, NULL, 6, NULL);

 button_init();
}
