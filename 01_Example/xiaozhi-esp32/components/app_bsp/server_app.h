#pragma once

#include <freertos/FreeRTOS.h>
#include "sdcard_bsp.h"
#include "traverse_nvs.h"

extern EventGroupHandle_t ServerPortGroups;


uint8_t ServerPort_NetworkInit(wifi_credential_t creden);
void ServerPort_init(CustomSDPort *SDPort);
void ServerPort_SetNetworkSleep(void);


