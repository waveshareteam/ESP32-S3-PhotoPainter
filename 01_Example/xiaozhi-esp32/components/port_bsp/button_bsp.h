#ifndef BUTTON_BSP_H
#define BUTTON_BSP_H

#include <freertos/FreeRTOS.h>

#ifdef __cplusplus
extern "C" {
#endif

extern EventGroupHandle_t BootButtonGroups;
extern EventGroupHandle_t GP4ButtonGroups;
extern EventGroupHandle_t PWRButtonGroups;


#define GroupSetBitsMax 0x00ffffff
#define GroupBit0      0x00000001
#define GroupBit1      0x00000002
#define GroupBit2      0x00000004
#define GroupBit3      0x00000008
#define GroupBit4      0x00000010
#define GroupBit5      0x00000020
#define GroupBit6      0x00000040
#define GroupBit7      0x00000080


void Custom_ButtonInit(void);

#ifdef __cplusplus
}
#endif

#endif