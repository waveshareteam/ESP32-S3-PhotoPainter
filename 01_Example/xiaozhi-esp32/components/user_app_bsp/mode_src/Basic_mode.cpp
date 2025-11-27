#include "axp_prot.h"
#include "button_bsp.h"
#include "driver/rtc_io.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "json_data.h"
#include "led_bsp.h"
#include "sdcard_bsp.h"
#include "user_app.h"
#include <stdio.h>

#include "GUI_BMPfile.h"
#include "GUI_Paint.h"
#include "epaper_port.h"

#define ext_wakeup_pin_1 GPIO_NUM_0 
#define ext_wakeup_pin_2 GPIO_NUM_5 
#define ext_wakeup_pin_3 GPIO_NUM_4 

static uint8_t *epd_blackImage = NULL; // Image buffer
static uint32_t Imagesize;             // Size of image buffer

static uint32_t               sdcard_Basic_bmp   = 0; 
static RTC_DATA_ATTR uint32_t sdcard_Basic_count = 0; 


static RTC_DATA_ATTR int basic_rtc_set_time = 13 * 60;// User sets the wake-up time in seconds. // The default is 60 seconds. It is awakened by a timer.

static uint8_t           Basic_sleep_arg = 0; // Parameters for low-power tasks
static SemaphoreHandle_t sleep_Semp;          // Binary call low-power task
 
static uint8_t           wakeup_basic_flag = 0;

static void pwr_button_user_Task(void *arg) {
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(pwr_groups, set_bit_all, pdTRUE,
                                               pdFALSE, pdMS_TO_TICKS(2000));
        if (get_bit_button(even, 0)) // Immediately enter low-power mode
        {
            esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_AUTO);
            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);     
            const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;
            const uint64_t ext_wakeup_pin_3_mask = 1ULL << ext_wakeup_pin_3;
            ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup_io(ext_wakeup_pin_1_mask | ext_wakeup_pin_3_mask, ESP_EXT1_WAKEUP_ANY_LOW)); 
            ESP_ERROR_CHECK(rtc_gpio_pulldown_dis(ext_wakeup_pin_3));
            ESP_ERROR_CHECK(rtc_gpio_pullup_en(ext_wakeup_pin_3));
            esp_sleep_enable_timer_wakeup(basic_rtc_set_time * 1000 * 1000);
            //axp_basic_sleep_start();
            vTaskDelay(pdMS_TO_TICKS(500));
            esp_deep_sleep_start(); 
        }
    }
}

static void boot_button_user_Task(void *arg) {
    Imagesize      = ((EXAMPLE_LCD_WIDTH % 2 == 0) ? (EXAMPLE_LCD_WIDTH / 2) : (EXAMPLE_LCD_WIDTH / 2 + 1)) * EXAMPLE_LCD_HEIGHT;
    epd_blackImage = (uint8_t *) heap_caps_malloc(Imagesize * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
    assert(epd_blackImage);
    /*刷图的公共部分*/
    Paint_NewImage(epd_blackImage, EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT, 0, EPD_7IN3E_WHITE);
    Paint_SetScale(6);
    Paint_SelectImage(epd_blackImage); 
    Paint_SetRotate(180);

    uint8_t *wakeup_arg = (uint8_t *) arg;
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(boot_groups, set_bit_all, pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
        if (get_bit_button(even, 0)) {
            if (*wakeup_arg == 0) {
                if (pdTRUE == xSemaphoreTake(epaper_gui_semapHandle, 2000)) {                       
                    list_node_t *sdcard_node = list_at(sdcard_scan_listhandle, sdcard_Basic_count); 
                    if (sdcard_node == NULL) {
                        sdcard_Basic_count = 0;
                        sdcard_node        = list_at(sdcard_scan_listhandle, sdcard_Basic_count);
                    }
                    ESP_LOGE("node", "%ld", sdcard_Basic_count);
                    sdcard_Basic_count++;
                    if (sdcard_node != NULL) 
                    {
                        
                        xEventGroupSetBits(Green_led_Mode_queue,
                                           set_bit_button(6));
                        Green_led_arg                   = 1;
                        sdcard_node_t *sdcard_Name_node = (sdcard_node_t *) sdcard_node->val;
                        GUI_ReadBmp_RGB_6Color(sdcard_Name_node->sdcard_name, 0, 0);
                        epaper_port_display(epd_blackImage);    
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
                esp_sleep_pd_config(
                    ESP_PD_DOMAIN_MAX,
                    ESP_PD_OPTION_AUTO);   
                esp_sleep_disable_wakeup_source(
                    ESP_SLEEP_WAKEUP_ALL); 
                const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;
                const uint64_t ext_wakeup_pin_3_mask = 1ULL << ext_wakeup_pin_3;
                ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup_io(
                    ext_wakeup_pin_1_mask | ext_wakeup_pin_3_mask,
                    ESP_EXT1_WAKEUP_ANY_LOW)); 
                ESP_ERROR_CHECK(rtc_gpio_pulldown_dis(ext_wakeup_pin_3));
                ESP_ERROR_CHECK(rtc_gpio_pullup_en(ext_wakeup_pin_3));
                esp_sleep_enable_timer_wakeup(basic_rtc_set_time * 1000 * 1000);
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
            // esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
            // //Disable the previous timer first
            // esp_sleep_enable_timer_wakeup(basic_rtc_set_time * 1000 * 1000);
            // //Reset the 10-second timer
            xEventGroupSetBits(boot_groups, set_bit_button(0)); 
        } else if (wakeup_pins & (1ULL << ext_wakeup_pin_3)) {
            return;
        }
    } else if (ESP_SLEEP_WAKEUP_TIMER == wakeup_reason) {
        xEventGroupSetBits(boot_groups, set_bit_button(0)); 
    }
}

void User_Basic_mode_app_init(void) {
    sleep_Semp  = xSemaphoreCreateBinary();
    xEventGroupSetBits(Red_led_Mode_queue, set_bit_button(0));  
    ai_model_t *ai_model_data = NULL;
    if ((13 * 60) == basic_rtc_set_time) {
        ai_model_data = json_sdcard_txt_aimodel();
        if (ai_model_data != NULL) {                            
            basic_rtc_set_time = ai_model_data->time;
            ESP_LOGI("TIMER", "basic_rtc_set_time:%d", basic_rtc_set_time);
        }
    }
    if(ai_model_data != NULL) {free(ai_model_data);ai_model_data = NULL;}
    list_scan_dir("/sdcard/06_user_foundation_img");        
    sdcard_Basic_bmp = list_iterator();
    xTaskCreate(boot_button_user_Task, "boot_button_user_Task", 6 * 1024, &wakeup_basic_flag, 3, NULL);
    xTaskCreate(pwr_button_user_Task, "pwr_button_user_Task", 4 * 1024, NULL, 3, NULL);
    xTaskCreate(default_sleep_user_Task, "default_sleep_user_Task", 4 * 1024, &Basic_sleep_arg, 3, NULL); 
    get_wakeup_gpio();
}

