#include <stdio.h>
#include <string.h>
#include <esp_heap_caps.h>
#include <nvs_flash.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include "user_app.h"
#include "button_bsp.h"
#include "ai_app.h"
#include "list.h"


#define ext_wakeup_pin_1 GPIO_NUM_0 
#define ext_wakeup_pin_2 GPIO_NUM_5 
#define ext_wakeup_pin_3 GPIO_NUM_4 

static RTC_DATA_ATTR uint32_t sdcard_Basic_count = 0; 
static RTC_DATA_ATTR int basic_rtc_set_time = 13 * 60;// User sets the wake-up time in seconds. // The default is 60 seconds. It is awakened by a timer.
static uint8_t           Basic_sleep_arg = 0; // Parameters for low-power tasks
static SemaphoreHandle_t sleep_Semp;          // Binary call low-power task 
static uint8_t           wakeup_basic_flag = 0;
static list_t* ListHost;



static void pwr_button_user_Task(void *arg) {
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(PWRButtonGroups, set_bit_all, pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
        if (get_bit_button(even, 0))
        {
            const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;
            const uint64_t ext_wakeup_pin_3_mask = 1ULL << ext_wakeup_pin_3;
            ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup_io(ext_wakeup_pin_1_mask | ext_wakeup_pin_3_mask, ESP_EXT1_WAKEUP_ANY_LOW)); 
            ESP_ERROR_CHECK(rtc_gpio_pulldown_dis(ext_wakeup_pin_3));
            ESP_ERROR_CHECK(rtc_gpio_pullup_en(ext_wakeup_pin_3));
            esp_sleep_enable_timer_wakeup((uint64_t)basic_rtc_set_time * 1000000ULL);
            //axp_basic_sleep_start();
            vTaskDelay(pdMS_TO_TICKS(500));
            esp_deep_sleep_start(); 
        }
    }
}

static void boot_button_user_Task(void *arg) {
    uint8_t *wakeup_arg = (uint8_t *) arg;
    ePaperDisplay.EPD_Init();
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(BootButtonGroups, set_bit_all, pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
        if (get_bit_button(even, 0)) {
            if (*wakeup_arg == 0) {
                if (pdTRUE == xSemaphoreTake(epaper_gui_semapHandle, 2000)) {                       
                    list_node_t *sdcard_node = list_at(ListHost, sdcard_Basic_count); 
                    if (sdcard_node == NULL) {
                        sdcard_Basic_count = 0;
                        sdcard_node        = list_at(ListHost, sdcard_Basic_count);
                    }
                    ESP_LOGW("node", "%ld", sdcard_Basic_count);
                    sdcard_Basic_count++;
                    if (sdcard_node != NULL) 
                    {
                        xEventGroupSetBits(Green_led_Mode_queue,set_bit_button(6));
                        Green_led_arg                   = 1;
                        CustomSDPortNode_t *sdcard_Name_node = (CustomSDPortNode_t *) sdcard_node->val;
                        ePaperDisplay.EPD_SDcardBmpShakingColor(sdcard_Name_node->sdcard_name,0,0);
                        ePaperDisplay.EPD_Display();
                        xSemaphoreGive(epaper_gui_semapHandle); 
                        Green_led_arg = 0;
                        xSemaphoreGive(sleep_Semp);
                        Basic_sleep_arg = 1;
                    }
                }
            }
        }
    }
}

static void default_sleep_user_Task(void *arg) {
    uint8_t *sleep_arg = (uint8_t *) arg;
    for (;;) {
        if (pdTRUE == xSemaphoreTake(sleep_Semp, portMAX_DELAY)) {
            if (*sleep_arg == 1) {
                const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;
                const uint64_t ext_wakeup_pin_3_mask = 1ULL << ext_wakeup_pin_3;
                ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup_io(ext_wakeup_pin_1_mask | ext_wakeup_pin_3_mask,ESP_EXT1_WAKEUP_ANY_LOW)); 
                ESP_ERROR_CHECK(rtc_gpio_pulldown_dis(ext_wakeup_pin_3));
                ESP_ERROR_CHECK(rtc_gpio_pullup_en(ext_wakeup_pin_3));
                esp_sleep_enable_timer_wakeup((uint64_t)basic_rtc_set_time * 1000000ULL);
                //axp_basic_sleep_start(); 
                vTaskDelay(pdMS_TO_TICKS(500));
                esp_deep_sleep_start();  
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
        if (wakeup_pins & (1ULL << ext_wakeup_pin_1)) {
            xEventGroupSetBits(BootButtonGroups, set_bit_button(0)); 
        } else if (wakeup_pins & (1ULL << ext_wakeup_pin_3)) {
            return;
        }
    } else if (ESP_SLEEP_WAKEUP_TIMER == wakeup_reason) {
        xEventGroupSetBits(BootButtonGroups, set_bit_button(0)); 
    }
}

void User_Basic_mode_app_init(void) {
    ListHost = SDPort->SDPort_GetListHost();
    sleep_Semp  = xSemaphoreCreateBinary();
    BaseAIModel model(SDPort);
    xEventGroupSetBits(Red_led_Mode_queue, set_bit_button(0));  
    ai_model_t *ai_model_data = NULL;
    //if ((13 * 60) == basic_rtc_set_time) {
        ai_model_data = json_sdcard_txt_aimodel();
        if (ai_model_data != NULL) {                            
            basic_rtc_set_time = ai_model_data->time;
            ESP_LOGI("TIMER", "basic_rtc_set_time:%d", basic_rtc_set_time);
        }
    //}
    if(ai_model_data != NULL) {free(ai_model_data);ai_model_data = NULL;}
    list_scan_dir("/sdcard/06_user_foundation_img");        
    sdcard_Basic_bmp = list_iterator();
    xTaskCreate(boot_button_user_Task, "boot_button_user_Task", 6 * 1024, &wakeup_basic_flag, 3, NULL);
    xTaskCreate(pwr_button_user_Task, "pwr_button_user_Task", 4 * 1024, NULL, 3, NULL);
    xTaskCreate(default_sleep_user_Task, "default_sleep_user_Task", 4 * 1024, &Basic_sleep_arg, 3, NULL); 
    get_wakeup_gpio();
}

