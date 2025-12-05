#pragma once

#include <freertos/FreeRTOS.h>
#include "sdcard_bsp.h"

extern EventGroupHandle_t ServerPortGroups;


void ServerPort_init(CustomSDPort *SDPort);
void ServerPort_NetworkInit(void);
void ServerPort_SetNetworkSleep(void);


