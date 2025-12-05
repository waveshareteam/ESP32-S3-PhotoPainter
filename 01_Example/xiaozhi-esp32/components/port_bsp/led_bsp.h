#ifndef LED_BSP_H
#define LED_BSP_H

#include <freertos/FreeRTOS.h> 


#define LED_PIN_Red   GPIO_NUM_45
#define LED_PIN_Green GPIO_NUM_42
#define LED_ON  0
#define LED_OFF 1

extern EventGroupHandle_t Led_Groups;

#ifdef __cplusplus
extern "C" {
#endif


void Led_init(void);
void Led_SetLevel(uint8_t led,uint8_t mode);
void Led_RegLoopTask(void *arg);
void Led_GreenLoopTask(void *arg);
void Led_SetFlicker(uint8_t led, uint8_t mode);


#ifdef __cplusplus
}
#endif

#endif 

