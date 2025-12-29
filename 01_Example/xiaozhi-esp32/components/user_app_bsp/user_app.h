#pragma once

#include <freertos/FreeRTOS.h>
#include "sdcard_bsp.h"
#include "display_bsp.h"
#include "i2c_bsp.h"

extern CustomSDPort *SDPort;
extern ePaperPort ePaperDisplay;
extern I2cMasterBus I2cBus;

uint8_t User_Mode_init(void);       // main.cc

extern EventGroupHandle_t Green_led_Mode_queue; 
extern EventGroupHandle_t Red_led_Mode_queue; 
extern SemaphoreHandle_t epaper_gui_semapHandle;
extern uint8_t Green_led_arg;           
extern uint8_t Red_led_arg;
extern int img_loopTimer;            
extern EventGroupHandle_t epaper_groups;
extern EventGroupHandle_t ai_IMG_LoopGroup;

void User_xiaozhi_app_init(void); // init
void xiaozhi_init_received(const char *arg1);
void xiaozhi_ai_Message(const char *arg1, const char *arg2);
void xiaozhi_application_received(const char *str);
char* Get_TemperatureHumidity(void);
extern int sdcard_bmp_Quantity;
extern int sdcard_doc_count; 
extern int is_ai_img;        
extern EventGroupHandle_t ai_IMG_Group;
extern SemaphoreHandle_t ai_img_while_semap;


void User_Basic_mode_app_init(void);


void User_Network_mode_app_init(void);


void Mode_Selection_Init(void);
