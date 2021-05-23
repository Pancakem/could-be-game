#ifndef _HTTP_COMM_H
#define _HTTP_COMM_H

#define WEB_SERVER ""
#define WEB_PORT ""
#define WEB_URL ""

char *prepare_http_request(char *data);
void https_get_task(void *pvParameters);

#endif
