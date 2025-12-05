#include <driver/gpio.h>
#include <stdio.h>
#include "led_bsp.h"
#include "button_bsp.h"

EventGroupHandle_t Led_Groups;

void Led_init(void) {
    Led_Groups              = xEventGroupCreate();
    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type     = GPIO_INTR_DISABLE;
    gpio_conf.mode          = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask  = (0x1ULL << LED_PIN_Red) | (0x1ULL << LED_PIN_Green);
    gpio_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en    = GPIO_PULLUP_ENABLE;

    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
    Led_SetLevel(LED_PIN_Red, LED_OFF);
    Led_SetLevel(LED_PIN_Green, LED_OFF);
}

void Led_SetLevel(uint8_t led, uint8_t mode) {
    gpio_set_level(led, mode);
}

void Led_RegLoopTask(void *arg) {
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(Led_Groups, (0x01UL<<0) | (0x01UL<<1) | (0x01UL<<2), pdFALSE, pdFALSE, 1000);
        if (even & 0x01) //慢闪
        {
            Led_SetLevel(LED_PIN_Red, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
            Led_SetLevel(LED_PIN_Red, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else if ((even>>1) & 0x01) //中闪
        {
            Led_SetLevel(LED_PIN_Red, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            Led_SetLevel(LED_PIN_Red, 1);
            vTaskDelay(pdMS_TO_TICKS(500));
        } else if ((even>>2) & 0x01) //快闪
        {
            Led_SetLevel(LED_PIN_Red, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            Led_SetLevel(LED_PIN_Red, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            Led_SetLevel(LED_PIN_Red, 0);
        }
    }
}


void Led_GreenLoopTask(void *arg) {
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(Led_Groups, (0x01UL<<3) | (0x01UL<<4) | (0x01UL<<5), pdFALSE, pdFALSE, 1000);
        if ((even>>3) & 0x01) //慢闪
        {
            Led_SetLevel(LED_PIN_Green, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
            Led_SetLevel(LED_PIN_Green, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else if ((even>>4) & 0x01) //中闪
        {
            Led_SetLevel(LED_PIN_Green, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            Led_SetLevel(LED_PIN_Green, 1);
            vTaskDelay(pdMS_TO_TICKS(500));
        } else if ((even>>5) & 0x01) //快闪
        {
            Led_SetLevel(LED_PIN_Green, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            Led_SetLevel(LED_PIN_Green, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            Led_SetLevel(LED_PIN_Green, 1);
        }
    }
}

void Led_SetFlicker(uint8_t led, uint8_t mode) {
    if (led == LED_PIN_Green) {
        xEventGroupClearBits(Led_Groups, (0x01UL<<3) | (0x01UL<<4) | (0x01UL<<5));
        switch (mode) {
        case 1:
            xEventGroupSetBits(Led_Groups, (0x01UL<<5));
            break;
        case 2:
            xEventGroupSetBits(Led_Groups, (0x01UL<<4));
            break;
        case 3:
            xEventGroupSetBits(Led_Groups, (0x01UL<<3));
            break;
        default:
            break;
        }
    } else if (led == LED_PIN_Red) {
        xEventGroupClearBits(Led_Groups, (0x01UL<<0) | (0x01UL<<1) | (0x01UL<<2));
        switch (mode) {
        case 1:
            xEventGroupSetBits(Led_Groups, (0x01UL<<2)); //快
            break;
        case 2:
            xEventGroupSetBits(Led_Groups, (0x01UL<<1));
            break;
        case 3:
            xEventGroupSetBits(Led_Groups, (0x01UL<<0));
            break;
        default:
            break;
        }
    } else {
        xEventGroupClearBits(Led_Groups, (0x01UL<<0) | (0x01UL<<1) | (0x01UL<<2) | (0x01UL<<3) | (0x01UL<<4) | (0x01UL<<5));
    }
}
