#include "axp_prot.h"
#include "button_bsp.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "i2c_bsp.h"
#include "led_bsp.h"
#include "sdcard_bsp.h"
#include "user_app.h"
#include <stdio.h>
#include <string.h>

#include "GUI_BMPfile.h"
#include "GUI_Paint.h"
#include "epaper_port.h"

#include "nvs_flash.h"

#include "user_audio_bsp.h"

user_audio_bsp *dev_audio = NULL;

EventGroupHandle_t audio_groups;

static void key1_button_user_Task(void *arg) {
    esp_err_t ret;
    uint8_t   Mode = 0;
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(key_groups, set_bit_all, pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
        if (get_bit_button(even, 1)) {
            if (Mode > 0) {
                nvs_handle_t my_handle;
                ret = nvs_open("PhotoPainter", NVS_READWRITE, &my_handle);
                ESP_ERROR_CHECK(ret);
                vTaskDelay(pdMS_TO_TICKS(2));//ESP_LOGE("OK", "1");
                if (Mode == 1) {
                    ret = nvs_set_u8(my_handle, "PhotPainterMode", 0x01);
                    ESP_ERROR_CHECK(ret);
                    vTaskDelay(pdMS_TO_TICKS(2));//ESP_LOGE("OK", "2");
                } else if (Mode == 2) {
                    ret = nvs_set_u8(my_handle, "PhotPainterMode", 0x02);
                    ESP_ERROR_CHECK(ret);
                    vTaskDelay(pdMS_TO_TICKS(2));//ESP_LOGE("OK", "3");
                } else if (Mode == 3) {
                    ret = nvs_set_u8(my_handle, "PhotPainterMode", 0x03);
                    ESP_ERROR_CHECK(ret);
                    vTaskDelay(pdMS_TO_TICKS(2));//ESP_LOGE("OK", "4");
                }
                ret = nvs_set_u8(my_handle, "Mode_Flag", 0x01);
                ESP_ERROR_CHECK(ret);
                vTaskDelay(pdMS_TO_TICKS(2));//ESP_LOGE("OK", "5");
                ESP_LOGE("OK","0x%02x,0x%02x",dev_audio->Get_CodecReg("es8311",0x00),dev_audio->Get_CodecReg("es7210",0x00));
                //dev_audio->Set_CodecReg("es8311", 0x00, 0x1f);
                //dev_audio->Set_CodecReg("es7210", 0x00, 0x32);
                uint8_t regs = dev_audio->Get_CodecReg("es8311",0xfa);
                ESP_LOGE("es8311 reg","0x%02x",regs);
                dev_audio->Set_CodecReg("es8311", 0xfa, regs | 0x01);
                regs = dev_audio->Get_CodecReg("es7210",0x00);
                ESP_LOGE("es7210 reg","0x%02x",regs);
                dev_audio->Set_CodecReg("es7210", 0x00, regs | 0x06);
                ESP_ERROR_CHECK(nvs_commit(my_handle));
                nvs_close(my_handle); 
                vTaskDelay(pdMS_TO_TICKS(300));//ESP_LOGE("OK", "4");
                dev_audio->Set_CodecReg("es8311", 0xfa, 0x00);
                dev_audio->Set_CodecReg("es7210", 0x00, 0x32);
                esp_restart();
            }
        } else if (get_bit_button(even, 0)) { 
            Mode++;
            if (Mode > 3) {
                Mode = 1;
            }
            if (Mode == 1) {
                xEventGroupSetBits(audio_groups, set_bit_button(1));
            } else if (Mode == 2) {
                xEventGroupSetBits(audio_groups, set_bit_button(2));
            } else if (Mode == 3) {
                xEventGroupSetBits(audio_groups, set_bit_button(3));
            }
        }
    }
}

static void audio_user_Task(void *arg) {
    dev_audio->Play_InfoAudio();
    int value = 0;
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(audio_groups, set_bit_all, pdTRUE, pdFALSE, pdMS_TO_TICKS(3000));
        if (get_bit_button(even, 0)) { 
            value = 0;
        } else if (get_bit_button(even, 1)) {
            value = 1;
        } else if (get_bit_button(even, 2)) {
            value = 2;
        } else if (get_bit_button(even, 3)) {
            value = 3;
        }
        int      bytes_write = 0;
        int      bytes_sizt  = dev_audio->Get_MusicSizt(value);
        uint8_t *Music_ptr   = dev_audio->Get_MusicData(value);
        do {
            dev_audio->Play_BackWrite(Music_ptr, 256);
            Music_ptr += 256;
            bytes_write += 256;
        } while ((bytes_write < bytes_sizt) && (gpio_get_level(GPIO_NUM_4)));
    }
}

void Mode_Selection_Init(void) {
    dev_audio    = new user_audio_bsp();
    audio_groups = xEventGroupCreate();
    xEventGroupSetBits(audio_groups, set_bit_button(0));
    xTaskCreate(key1_button_user_Task, "key1_button_user_Task", 4 * 1024, NULL, 3, NULL);
    xTaskCreate(audio_user_Task, "audio_user_Task", 4 * 1024, NULL, 3, NULL);
}