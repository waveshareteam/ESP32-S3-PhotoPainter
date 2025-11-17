#ifndef SERVER_BSP_H
#define SERVER_BSP_H

#include "freertos/FreeRTOS.h"

extern EventGroupHandle_t server_groups;

#ifdef __cplusplus
extern "C" {
#endif

void http_server_init(void);
void Network_wifi_ap_init(void);
void set_espWifi_sleep(void);


#ifdef __cplusplus
}
#endif

#endif // !CLIENT_BSP_H
