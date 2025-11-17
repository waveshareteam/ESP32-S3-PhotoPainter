#ifndef LED_BSP_H
#define LED_BSP_H

#include "freertos/FreeRTOS.h"

#define LED_PIN_Red   45
#define LED_PIN_Green 42
#define LED_ALL_OFF_PIN 255  

#define LED_ON  0
#define LED_OFF 1

#ifdef __cplusplus
extern "C" {
#endif

void led_init(void);
void led_set(uint8_t led,uint8_t mode);
void led_Reg_loop_task(void *arg);
void led_Green_loop_task(void *arg);
void led_set_Flicker(uint8_t led,uint8_t mode);


#ifdef __cplusplus
}
#endif

#endif 

