//#ifndef _NTP_H
//#define _NTP_H

#include <time.h>


static const char *SNTP_CODE = "SNTP";
void time_sync_notification_cb(struct timeval *tv);
void initialize_sntp(void);
void obtain_time(void);

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM

void sntp_sync_time(struct timeval *tv);

#endif

// #endif
