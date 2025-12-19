#include <stdio.h>
#include <string.h>
#include <esp_heap_caps.h>
#include <nvs_flash.h>
#include <driver/rtc_io.h>
#include "user_app.h"
#include "led_bsp.h"
#include "button_bsp.h"
#include "power_bsp.h"
#include "led_bsp.h"

CustomSDPort *SDPort = NULL;
ePaperPort ePaperDisplay(11,10,8,9,12,13,800,480);
I2cMasterBus I2cBus(48,47,0);

SemaphoreHandle_t  epaper_gui_semapHandle = NULL; // Mutual exclusion lock to prevent repeated refreshing
EventGroupHandle_t epaper_groups;                 // Event group for map refreshing
EventGroupHandle_t Green_led_Mode_queue = 0;      // Queue for LED blinking, mainly for storing mode parameters
EventGroupHandle_t Red_led_Mode_queue   = 0;      // Queue for LED blinking, mainly for storing mode parameters
uint8_t            Green_led_arg        = 0;      // Parameters for LED task
uint8_t            Red_led_arg          = 0;      // Parameters for LED task

static void Green_led_user_Task(void *arg) {
    uint8_t *led_arg = (uint8_t *) arg;
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(Green_led_Mode_queue, set_bit_all, pdFALSE, pdFALSE, portMAX_DELAY);
        if (get_bit_data(even, 1)) {
            Led_SetLevel(LED_PIN_Green, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_OFF);
            xEventGroupClearBits(Green_led_Mode_queue, rset_bit_data(1));
        }
        if (get_bit_data(even, 2)) {
            Led_SetLevel(LED_PIN_Green, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_OFF);
            xEventGroupClearBits(Green_led_Mode_queue, rset_bit_data(2));
        }
        if (get_bit_data(even, 3)) {
            Led_SetLevel(LED_PIN_Green, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_OFF);
            xEventGroupClearBits(Green_led_Mode_queue, rset_bit_data(3));
        }
        if (get_bit_data(even, 4)) {
            Led_SetLevel(LED_PIN_Green, LED_ON);
            xEventGroupClearBits(Green_led_Mode_queue, rset_bit_data(4));
        }
        if (get_bit_data(even, 5)) {
            Led_SetLevel(LED_PIN_Green, LED_OFF);
            xEventGroupClearBits(Green_led_Mode_queue, rset_bit_data(5));
        }
        if (get_bit_data(even, 6)) {
            while (*led_arg) {
                Led_SetLevel(LED_PIN_Green, LED_ON);
                vTaskDelay(pdMS_TO_TICKS(100));
                Led_SetLevel(LED_PIN_Green, LED_OFF);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            xEventGroupClearBits(Green_led_Mode_queue, rset_bit_data(6));
        }
        if (get_bit_data(even, 7)) 
        {
            Led_SetLevel(LED_PIN_Green, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_ON);
            vTaskDelay(pdMS_TO_TICKS(200));
            Led_SetLevel(LED_PIN_Green, LED_OFF);
            xEventGroupClearBits(Green_led_Mode_queue, rset_bit_data(7));
        }
    }
}

static void Red_led_user_Task(void *arg) {
    uint8_t *led_arg = (uint8_t *) arg;
    for (;;) {
        EventBits_t even =
            xEventGroupWaitBits(Red_led_Mode_queue, set_bit_all, pdFALSE, pdFALSE, portMAX_DELAY);
        if (get_bit_data(even, 0)) {
            Led_SetLevel(LED_PIN_Red, LED_ON);
            xEventGroupClearBits(Red_led_Mode_queue, rset_bit_data(0));
        }
        if (get_bit_data(even, 6)) {
            while (*led_arg) {
                vTaskDelay(pdMS_TO_TICKS(100));
                Led_SetLevel(LED_PIN_Red, LED_OFF);
                vTaskDelay(pdMS_TO_TICKS(100));
                Led_SetLevel(LED_PIN_Red, LED_ON);
            }
            xEventGroupClearBits(Red_led_Mode_queue, rset_bit_data(6));
        }
    }
}

static void key1_button_user_Task(void *arg) {
    esp_err_t ret;
    for (;;) {
        EventBits_t even = xEventGroupWaitBits(GP4ButtonGroups, (0x02), pdFALSE, pdFALSE, pdMS_TO_TICKS(2000));
        if (get_bit_button(even, 1)) { 
            nvs_handle_t my_handle;
            ret = nvs_open("PhotoPainter", NVS_READWRITE, &my_handle);
            ESP_ERROR_CHECK(ret);
            uint8_t Mode_value = 0;
            ret                = nvs_get_u8(my_handle, "Mode_Flag", &Mode_value);
            ESP_ERROR_CHECK(ret);
            if (Mode_value == 0x01) { 
                xEventGroupClearBits(GP4ButtonGroups, set_bit_button(1));
                ret = nvs_set_u8(my_handle, "Mode_Flag", 0x00);
                ESP_ERROR_CHECK(ret);
                ret = nvs_set_u8(my_handle, "PhotPainterMode", 0x04);
                ESP_ERROR_CHECK(ret);
                nvs_commit(my_handle);
                nvs_close(my_handle); 
                esp_restart();
            }
        }
    }
}

uint8_t User_Mode_init(void) 
{
    epaper_gui_semapHandle = xSemaphoreCreateMutex(); /* Acquire the mutual exclusion lock to prevent re-flashing */
    Custom_PmicPortInit(&I2cBus,0x34);
    Led_init();                                       /* LED Blink Initialization */
    SDPort = new CustomSDPort("/sdcard");
    uint8_t sdcard_win = SDPort->SDPort_GetSdcardInitOK();              /* SD Card Initialization */
    if (sdcard_win == 0)
        return 0;
    Green_led_Mode_queue = xEventGroupCreate();
    Red_led_Mode_queue   = xEventGroupCreate();
    epaper_groups        = xEventGroupCreate();
    /*GPIO */
    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type     = GPIO_INTR_DISABLE;
    gpio_conf.mode          = GPIO_MODE_INPUT;
    gpio_conf.pin_bit_mask  = 0x1ULL << GPIO_NUM_4;
    gpio_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en    = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
    do {
        vTaskDelay(pdMS_TO_TICKS(50)); 
    } while (!gpio_get_level(GPIO_NUM_4));
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_reset_pin(GPIO_NUM_4));
    Custom_ButtonInit();
    xTaskCreate(key1_button_user_Task, "key1_button_user_Task", 4 * 1024, NULL, 3, NULL);
    xTaskCreate(Green_led_user_Task, "Green_led_user_Task", 3 * 1024, &Green_led_arg, 2, NULL);
    xTaskCreate(Red_led_user_Task, "Red_led_user_Task", 3 * 1024, &Red_led_arg, 2, NULL);
    xTaskCreate(Axp2101_isChargingTask, "Axp2101_isChargingTask", 3 * 1024, NULL, 2, NULL);   //AXP2101 Charging
    return 1;
}
