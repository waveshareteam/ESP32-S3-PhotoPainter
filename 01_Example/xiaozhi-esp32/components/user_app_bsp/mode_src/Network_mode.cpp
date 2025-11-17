#include "axp_prot.h"
#include "button_bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "led_bsp.h"
#include "server_bsp.h"
#include "user_app.h"
#include <stdio.h>

#include "GUI_BMPfile.h"
#include "GUI_Paint.h"
#include "epaper_port.h"

#include "driver/rtc_io.h"

#define ext_wakeup_pin_3 GPIO_NUM_4

static EventGroupHandle_t sleep_group;
static uint8_t           *epd_blackImage = NULL; // Image buffer
static uint32_t           Imagesize;             // Size of image buffer

static void Network_user_Task(void *arg) {
    Imagesize =
        ((EXAMPLE_LCD_WIDTH % 2 == 0) ? (EXAMPLE_LCD_WIDTH / 2) : (EXAMPLE_LCD_WIDTH / 2 + 1)) * EXAMPLE_LCD_HEIGHT;
    epd_blackImage = (uint8_t *) heap_caps_malloc(Imagesize * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
    assert(epd_blackImage);
    
    Paint_NewImage(epd_blackImage, EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT, 0, EPD_7IN3E_WHITE);
    Paint_SetScale(6);
    Paint_SelectImage(epd_blackImage);
    Paint_SetRotate(180);
    for (;;) {
        EventBits_t even =
            xEventGroupWaitBits(server_groups, set_bit_all, pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
        if (get_bit_button(even, 0)) 
        {
            Red_led_arg = 1;                                           
            xEventGroupSetBits(Red_led_Mode_queue, set_bit_button(6)); 
        } else if (get_bit_button(even, 1)) {
            Red_led_arg = 0;                
        } else if (get_bit_button(even, 2)) 
        {
            if (pdTRUE == xSemaphoreTake(epaper_gui_semapHandle,
                                         2000)) 
            {
                
                xEventGroupSetBits(Green_led_Mode_queue, set_bit_button(6));
                Green_led_arg = 1;
                GUI_ReadBmp_RGB_6Color("/sdcard/02_sys_ap_img/user_send.bmp", 0, 0);
                epaper_port_display(epd_blackImage);    
                xSemaphoreGive(epaper_gui_semapHandle); 
                Green_led_arg = 0;                      
            }
        } else if (get_bit_button(even, 5)) 
        {
            esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_AUTO); 
            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);      
            const uint64_t ext_wakeup_pin_3_mask = 1ULL << ext_wakeup_pin_3;
            ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup_io(
                ext_wakeup_pin_3_mask, ESP_EXT1_WAKEUP_ANY_LOW)); 
            ESP_ERROR_CHECK(rtc_gpio_pulldown_dis(ext_wakeup_pin_3));
            ESP_ERROR_CHECK(rtc_gpio_pullup_en(ext_wakeup_pin_3));
            esp_sleep_enable_timer_wakeup(30 * 1000 * 1000); 
            set_espWifi_sleep();                             
            //axp_basic_sleep_start();
            vTaskDelay(pdMS_TO_TICKS(500));                        
            esp_deep_sleep_start();                          
        } else if (get_bit_button(even, 4))                  
        {
            xEventGroupClearBits(sleep_group, rset_bit_data(0)); 
            xEventGroupSetBits(sleep_group, set_bit_button(1));  
        }
    }
}

static void Network_sleep_Task(void *arg) {
    size_t time = 0;
    for (;;) {
        EventBits_t even =
            xEventGroupWaitBits(sleep_group, set_bit_all, pdFALSE, pdFALSE, pdMS_TO_TICKS(1000));
        if (get_bit_button(even, 0)) 
        {
            vTaskDelay(pdMS_TO_TICKS(500));
            time++;
            if (time == 60) {
                esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_AUTO); 
                esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);      
                const uint64_t ext_wakeup_pin_3_mask = 1ULL << ext_wakeup_pin_3;
                ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup_io(
                    ext_wakeup_pin_3_mask, ESP_EXT1_WAKEUP_ANY_LOW)); 
                ESP_ERROR_CHECK(rtc_gpio_pulldown_dis(ext_wakeup_pin_3));
                ESP_ERROR_CHECK(rtc_gpio_pullup_en(ext_wakeup_pin_3));
                esp_sleep_enable_timer_wakeup(30 * 1000 * 1000); // 15s
                set_espWifi_sleep();                             
                //axp_basic_sleep_start(); 
                vTaskDelay(pdMS_TO_TICKS(500));                        
                esp_deep_sleep_start();                          
            }
        } else if (get_bit_button(even, 1)) 
        {
            time = 0;
            xEventGroupClearBits(sleep_group, rset_bit_data(1)); 
        }
    }
}

static void get_wakeup_gpio(void) {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (ESP_SLEEP_WAKEUP_EXT1 == wakeup_reason) {
        uint64_t wakeup_pins = esp_sleep_get_ext1_wakeup_status();
        if (wakeup_pins == 0)
            return;
        if (wakeup_pins & (1ULL << ext_wakeup_pin_3)) {
            xEventGroupClearBits(sleep_group, rset_bit_data(0)); 
        }
    } else if (ESP_SLEEP_WAKEUP_TIMER == wakeup_reason) {
    }
}

static void pwr_button_user_Task(void *arg) {
    for (;;) {
        EventBits_t even =
            xEventGroupWaitBits(pwr_groups, set_bit_all, pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
        if (get_bit_button(even, 0)) 
        {
            esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_AUTO); 
            esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);      
            const uint64_t ext_wakeup_pin_3_mask = 1ULL << ext_wakeup_pin_3;
            ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup_io(
                ext_wakeup_pin_3_mask, ESP_EXT1_WAKEUP_ANY_LOW)); 
            ESP_ERROR_CHECK(rtc_gpio_pulldown_dis(ext_wakeup_pin_3));
            ESP_ERROR_CHECK(rtc_gpio_pullup_en(ext_wakeup_pin_3));
            esp_sleep_enable_timer_wakeup(30 * 1000 * 1000);
            set_espWifi_sleep();     
            //axp_basic_sleep_start();
            vTaskDelay(pdMS_TO_TICKS(500)); 
            esp_deep_sleep_start();  
        }
    }
}

void User_Network_mode_app_init(void) {
    sleep_group = xEventGroupCreate();
    xEventGroupSetBits(sleep_group, set_bit_button(0)); 
    Network_wifi_ap_init();                             
    http_server_init();                                 
    xEventGroupSetBits(Red_led_Mode_queue,
                       set_bit_button(0)); 
    xTaskCreate(Network_user_Task, "Network_user_Task", 6 * 1024, NULL, 2, NULL);
    xTaskCreate(Network_sleep_Task, "Network_sleep_Task", 5 * 1024, NULL, 2, NULL);
    xTaskCreate(pwr_button_user_Task, "pwr_button_user_Task", 5 * 1024, NULL, 2, NULL);
    get_wakeup_gpio(); 
}
