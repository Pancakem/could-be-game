#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "http_comm.h"

static const char *HTTP_CODE = "HTTP_CLIENT";

char *prepare_http_request(char *data) {
  char *req = " HTTP/1.0\r\n"
    "Host: "WEB_SERVER"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "\r\n";
  char *param = malloc(64 * sizeof(char)); // remember this
  sprintf(param, "GET " WEB_URL "/%s", req);
  return param;
}

void https_get_task(void *pvParameters) {
  char buf[512];
  int ret, len;

  // the parameter should not be null
  configASSERT((char *) pvParameters != NULL);
  esp_tls_cfg_t cfg = {
                       .crt_bundle_attach = esp_crt_bundle_attach,
  };

  struct esp_tls *tls = esp_tls_conn_http_new(WEB_URL, &cfg);

  if (tls != NULL) {
    ESP_LOGI(HTTP_CODE, "Connection established ...");
  }else {
    ESP_LOGE(HTTP_CODE, "Connection failed ...");
    goto exit;
  }

  char *REQUEST = prepare_http_request((char *)pvParameters);
  size_t written_bytes = 0;
  do {
            ret = esp_tls_conn_write(tls,
                                      REQUEST + written_bytes,
                                     strlen(REQUEST) - written_bytes);
            if (ret >= 0) {
                ESP_LOGI(HTTP_CODE, "%d bytes written", ret);
                written_bytes += ret;
            } else if (ret != ESP_TLS_ERR_SSL_WANT_READ  && ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
                ESP_LOGE(HTTP_CODE, "esp_tls_conn_write  returned 0x%x", ret);
                goto exit;
            }
        } while(written_bytes < strlen(REQUEST));

        ESP_LOGI(HTTP_CODE, "Reading HTTP response...");

        do
        {
            len = sizeof(buf) - 1;
            bzero(buf, sizeof(buf));
            ret = esp_tls_conn_read(tls, (char *)buf, len);

            if(ret == ESP_TLS_ERR_SSL_WANT_WRITE  || ret == ESP_TLS_ERR_SSL_WANT_READ)
                continue;

            if(ret < 0)
           {
                ESP_LOGE(HTTP_CODE, "esp_tls_conn_read  returned -0x%x", -ret);
                break;
            }

            if(ret == 0)
            {
                ESP_LOGI(HTTP_CODE, "connection closed");
                break;
            }

            len = ret;
            ESP_LOGD(HTTP_CODE, "%d bytes read", len);
            /* Print response directly to stdout as it is read */
            for(int i = 0; i < len; i++) {
                putchar(buf[i]);
            }
        } while(1);

    exit:
        esp_tls_conn_delete(tls);
        putchar('\n'); // JSON output doesn't have a newline at end

        static int request_count;
        ESP_LOGI(HTTP_CODE, "Completed %d requests", ++request_count);

        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(HTTP_CODE, "%d...", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(HTTP_CODE, "Starting again!");
    }
