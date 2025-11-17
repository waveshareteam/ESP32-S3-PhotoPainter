#include "led_bsp.h"
#include "button_bsp.h"
#include "driver/gpio.h"
#include <stdio.h>

static EventGroupHandle_t led_groups;

void led_init(void) {
    led_groups              = xEventGroupCreate();
    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type     = GPIO_INTR_DISABLE;
    gpio_conf.mode          = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask  = ((uint64_t) 0x01 << LED_PIN_Red) | ((uint64_t) 0x01 << LED_PIN_Green);
    gpio_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en    = GPIO_PULLUP_ENABLE;

    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
    led_set(LED_PIN_Red, LED_OFF);
    led_set(LED_PIN_Green, LED_OFF);

    //xTaskCreate(led_Reg_loop_task, "led_Reg_loop_task", 2 * 1024, NULL, 3,NULL);
    //xTaskCreate(led_Green_loop_task, "led_Green_loop_task", 2 * 1024, NULL, 3,NULL);
}

void led_set(uint8_t led, uint8_t mode) {
    gpio_set_level(led, mode);
}

void led_Reg_loop_task(void *arg) {
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(led_groups, set_bit_button(0) | set_bit_button(1) | set_bit_button(2), pdFALSE, pdFALSE, 1000);
        if (get_bit_data(even, 0)) //慢闪
        {
            led_set(LED_PIN_Red, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
            led_set(LED_PIN_Red, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else if (get_bit_data(even, 1)) //中闪
        {
            led_set(LED_PIN_Red, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            led_set(LED_PIN_Red, 1);
            vTaskDelay(pdMS_TO_TICKS(500));
        } else if (get_bit_data(even, 2)) //快闪
        {
            led_set(LED_PIN_Red, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            led_set(LED_PIN_Red, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            led_set(LED_PIN_Red, 0);
        }
    }
}

void led_Green_loop_task(void *arg) {
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(led_groups, set_bit_button(3) | set_bit_button(4) | set_bit_button(5), pdFALSE, pdFALSE, 1000);
        if (get_bit_data(even, 3)) //慢闪
        {
            led_set(LED_PIN_Green, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
            led_set(LED_PIN_Green, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else if (get_bit_data(even, 4)) //中闪
        {
            led_set(LED_PIN_Green, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
            led_set(LED_PIN_Green, 1);
            vTaskDelay(pdMS_TO_TICKS(500));
        } else if (get_bit_data(even, 5)) //快闪
        {
            led_set(LED_PIN_Green, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            led_set(LED_PIN_Green, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            led_set(LED_PIN_Green, 1);
        }
    }
}

/*
  LED：LED序号 mode:闪烁速度
*/
void led_set_Flicker(uint8_t led, uint8_t mode) {
    if (led == LED_PIN_Green) {
        xEventGroupClearBits(led_groups, set_bit_button(3) | set_bit_button(4) | set_bit_button(5));
        switch (mode) {
        case 1:
            xEventGroupSetBits(led_groups, set_bit_button(5));
            break;
        case 2:
            xEventGroupSetBits(led_groups, set_bit_button(4));
            break;
        case 3:
            xEventGroupSetBits(led_groups, set_bit_button(3));
            break;
        default:
            break;
        }
    } else if (led == LED_PIN_Red) {
        xEventGroupClearBits(led_groups, set_bit_button(0) | set_bit_button(1) | set_bit_button(2));
        switch (mode) {
        case 1:
            xEventGroupSetBits(led_groups, set_bit_button(2)); //快
            break;
        case 2:
            xEventGroupSetBits(led_groups, set_bit_button(1));
            break;
        case 3:
            xEventGroupSetBits(led_groups, set_bit_button(0));
            break;
        default:
            break;
        }
    } else {
        xEventGroupClearBits(led_groups, set_bit_button(0) | set_bit_button(1) | set_bit_button(2) | set_bit_button(3) | set_bit_button(4) | set_bit_button(5));
    }
}
