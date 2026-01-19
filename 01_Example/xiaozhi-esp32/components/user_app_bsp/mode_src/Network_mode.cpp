#include <stdio.h>
#include <string.h>
#include <esp_heap_caps.h>
#include <nvs_flash.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include "display_bsp.h"
#include "server_app.h"
#include "button_bsp.h"
#include "user_app.h"
#include "traverse_nvs.h"

#define ext_wakeup_pin_4 GPIO_NUM_4

TraverseNvs *nvs_viewer = NULL;

static void Network_user_Task(void *arg) {
    ePaperDisplay.EPD_Init();
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(ServerPortGroups, (GroupBit0 | GroupBit1 | GroupBit2), pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
        if (even & GroupBit0) {
            Red_led_arg = 1;                                           
            xEventGroupSetBits(Red_led_Mode_queue, GroupBit6); 
        } else if (even & GroupBit1) {
            Red_led_arg = 0;                
        } else if (even & GroupBit2) {
            if (pdTRUE == xSemaphoreTake(epaper_gui_semapHandle,2000)) {
                xEventGroupSetBits(Green_led_Mode_queue, GroupBit6);
                Green_led_arg = 1;
                ePaperDisplay.EPD_SDcardBmpShakingColor("/sdcard/02_sys_ap_img/user_send.bmp",0,0);
                ePaperDisplay.EPD_Display();  
                xSemaphoreGive(epaper_gui_semapHandle); 
                Green_led_arg = 0;                      
            }
        }
    }
}

static void get_wakeup_gpio(void) {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (ESP_SLEEP_WAKEUP_EXT1 == wakeup_reason) {
        uint64_t wakeup_pins = esp_sleep_get_ext1_wakeup_status();
        if (wakeup_pins == 0)
            return;
        if (wakeup_pins & (1ULL << ext_wakeup_pin_4)) {
            
        }
    } else if (ESP_SLEEP_WAKEUP_TIMER == wakeup_reason) {
    }
}

static void pwr_button_user_Task(void *arg) {
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(PWRButtonGroups, GroupSetBitsMax, pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
        if (even & GroupBit0) {
            const uint64_t ext_wakeup_pin_4_mask = 1ULL << ext_wakeup_pin_4;
            ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup_io(ext_wakeup_pin_4_mask, ESP_EXT1_WAKEUP_ANY_LOW)); 
            ESP_ERROR_CHECK(rtc_gpio_pulldown_dis(ext_wakeup_pin_4));
            ESP_ERROR_CHECK(rtc_gpio_pullup_en(ext_wakeup_pin_4));
            ServerPort_SetNetworkSleep();     
            vTaskDelay(pdMS_TO_TICKS(500)); 
            esp_deep_sleep_start();  
        }
    }
}

void User_Network_mode_app_init(void) {
    nvs_viewer = new TraverseNvs();
    wifi_credential_t creden = nvs_viewer->Get_WifiCredentialFromNVS();
    if(0 == creden.is_valid) {
        xEventGroupSetBits(Red_led_Mode_queue,GroupBit1); 
        return;
    }
    uint8_t res = ServerPort_NetworkInit(creden); 
    if(0 == res) {
        return;
    }
    ServerPort_init(SDPort);
    xEventGroupSetBits(Red_led_Mode_queue,GroupBit0); 
    xTaskCreate(Network_user_Task, "Network_user_Task", 6 * 1024, NULL, 2, NULL);
    xTaskCreate(pwr_button_user_Task, "pwr_button_user_Task", 4 * 1024, NULL, 2, NULL);
    get_wakeup_gpio(); 
}
