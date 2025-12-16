#include <stdio.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>
#include "button_bsp.h"
#include "multi_button.h"

EventGroupHandle_t BootButtonGroups;
EventGroupHandle_t GP4ButtonGroups;
EventGroupHandle_t PWRButtonGroups;

static Button BootButton;   // Application button
#define BOOT_KEY_PIN 0      // Actual GPIO
#define BOOT_ID 1           // Button ID
#define BOOT_Active 0       // Valid level

static Button GP4Button; 
#define GP4_KEY_PIN 4    
#define GP4_ID 2         
#define GP4_Active 0     

static Button PWRButton; 
#define PWR_KEY_PIN 5    
#define PWR_ID 3         
#define PWR_Active 1     

/*******************Callback event declaration***************/
static void on_boot_single_click(Button *btn_handle) {
    xEventGroupSetBits(BootButtonGroups, set_bit_button(0));
}

static void on_boot_double_click(Button *btn_handle) {
    //xEventGroupSetBits(BootButtonGroups, set_bit_button(1));
}

static void on_boot_long_press_start(Button *btn_handle) {
    xEventGroupSetBits(BootButtonGroups, set_bit_button(1));
}

static void on_boot_press_repeat(Button *btn_handle) {
    xEventGroupSetBits(BootButtonGroups, set_bit_button(2));
}

static void on_boot_press_up(Button *btn_handle) {
    xEventGroupSetBits(BootButtonGroups, set_bit_button(3));
}

static void on_pwr_single_click(Button *btn_handle) {
    xEventGroupSetBits(PWRButtonGroups, set_bit_button(0));
}

static void on_gp4_single_click(Button *btn_handle) {
    xEventGroupSetBits(GP4ButtonGroups, set_bit_button(0));
}

static void on_gp4_double_click(Button *btn_handle) {
    //xEventGroupSetBits(GP4ButtonGroups, set_bit_button(1));
}

static void on_gp4_long_press_start(Button *btn_handle) {
    xEventGroupSetBits(GP4ButtonGroups, set_bit_button(1));
}

static void on_gp4_press_up(Button *btn_handle) {
    xEventGroupSetBits(GP4ButtonGroups, set_bit_button(2));
}

/*********************************************/

static void clock_task_callback(void *arg) {
    button_ticks();
}

static uint8_t read_button_GPIO(uint8_t Button_ID) {
    switch (Button_ID) {
    case BOOT_ID:
        return gpio_get_level(BOOT_KEY_PIN);
    case PWR_ID:
        return gpio_get_level(PWR_KEY_PIN);
    case GP4_ID:
        return gpio_get_level(GP4_KEY_PIN);
    default:
        break;
    }
    return 1;
}

static void gpio_init(void) {
    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type     = GPIO_INTR_DISABLE;
    gpio_conf.mode          = GPIO_MODE_INPUT;
    gpio_conf.pin_bit_mask  = (0x1ULL << BOOT_KEY_PIN) | (0x1ULL << GP4_KEY_PIN) | (0x1ULL << PWR_KEY_PIN);
    gpio_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en    = GPIO_PULLUP_ENABLE;

    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
}

void Custom_ButtonInit(void) {
    BootButtonGroups = xEventGroupCreate();
    PWRButtonGroups = xEventGroupCreate();
    GP4ButtonGroups = xEventGroupCreate();
    gpio_init();

    button_init(&BootButton, read_button_GPIO, BOOT_Active, BOOT_ID);           // Initialization: Initialize object, callback function, trigger level, key ID
    button_attach(&BootButton, BTN_SINGLE_CLICK, on_boot_single_click);         // Single click event
    button_attach(&BootButton, BTN_DOUBLE_CLICK, on_boot_double_click);         // Double click event
    button_attach(&BootButton, BTN_LONG_PRESS_START, on_boot_long_press_start); // Long press event
    button_attach(&BootButton, BTN_PRESS_UP, on_boot_press_up);                 //Release event
    button_attach(&BootButton, BTN_PRESS_REPEAT, on_boot_press_repeat);         //Continuous click event
    
    button_init(&PWRButton, read_button_GPIO, PWR_Active, PWR_ID);           
    button_attach(&PWRButton, BTN_SINGLE_CLICK, on_pwr_single_click);         
    //button_attach(&PWRButton, BTN_DOUBLE_CLICK, on_pwr_double_click);         
    //button_attach(&PWRButton, BTN_LONG_PRESS_START, on_pwr_long_press_start);

    button_init(&GP4Button, read_button_GPIO, GP4_Active, GP4_ID);           
    button_attach(&GP4Button, BTN_SINGLE_CLICK, on_gp4_single_click);         
    button_attach(&GP4Button, BTN_DOUBLE_CLICK, on_gp4_double_click);         
    button_attach(&GP4Button, BTN_LONG_PRESS_START, on_gp4_long_press_start);
    button_attach(&GP4Button, BTN_PRESS_UP, on_gp4_press_up);

    esp_timer_create_args_t clock_tick_timer_args = {};
    clock_tick_timer_args.callback                = &clock_task_callback;
    clock_tick_timer_args.name                    = "clock_task";
    clock_tick_timer_args.arg                     = NULL;
    esp_timer_handle_t clock_tick_timer           = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&clock_tick_timer_args, &clock_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(clock_tick_timer, 1000 * 5)); // 5ms
    button_start(&BootButton);
    button_start(&PWRButton);
    button_start(&GP4Button);
}

uint8_t user_boot_get_repeat_count(void) {
    return (button_get_repeat_count(&BootButton));
}
