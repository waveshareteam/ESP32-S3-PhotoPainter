#include "button_bsp.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "multi_button.h"

EventGroupHandle_t key_groups;
EventGroupHandle_t boot_groups;
EventGroupHandle_t pwr_groups;

static Button button1;   // Application button
#define USER_KEY_1 0     // Actual GPIO
#define button1_id 1     // Button ID
#define button1_active 0 // Valid level

static Button button2;   // Application button
#define USER_KEY_2 4     // Actual GPIO
#define button2_id 2     // Button ID
#define button2_active 0 // Valid level

static Button button3;   // Application button
#define USER_KEY_3 5     // Actual GPIO
#define button3_id 3     // Button ID
#define button3_active 1 // Valid level

/*******************Callback event declaration***************/

static void on_button2_press_repeat(Button *btn_handle);
static void on_button2_single_click(Button *btn_handle);
static void on_button2_double_click(Button *btn_handle);
static void on_button2_long_press_start(Button *btn_handle);
static void on_button2_long_press_hold(Button *btn_handle);
static void on_button2_press_down(Button *btn_handle);
static void on_button2_press_up(Button *btn_handle);

static void on_button1_single_click(Button *btn_handle);
static void on_button1_long_press_start(Button *btn_handle);
static void on_button1_press_repeat(Button *btn_handle);
static void on_button1_press_up(Button *btn_handle);

static void on_button3_single_click(Button *btn_handle);

/*********************************************/

static void clock_task_callback(void *arg) {
    button_ticks(); //Status callback
}

static uint8_t read_button_GPIO(uint8_t button_id) //Return the GPIO level
{
    switch (button_id) {
    case button1_id:
        return gpio_get_level(USER_KEY_1);
    case button2_id:
        return gpio_get_level(USER_KEY_2);
    case button3_id:
        return gpio_get_level(USER_KEY_3);
    default:
        break;
    }
    return 1;
}

static void gpio_init(void) {
    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type     = GPIO_INTR_DISABLE;
    gpio_conf.mode          = GPIO_MODE_INPUT;
    gpio_conf.pin_bit_mask  = ((uint64_t) 0x01 << USER_KEY_2) | ((uint64_t) 0x01 << USER_KEY_1);
    gpio_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en    = GPIO_PULLUP_ENABLE;

    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));

    gpio_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
    gpio_conf.pin_bit_mask = ((uint64_t) 0x01 << USER_KEY_3);

    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
}

void button_Init(void) {
    key_groups  = xEventGroupCreate();
    boot_groups = xEventGroupCreate();
    pwr_groups  = xEventGroupCreate();
    gpio_init();

    button_init(&button2, read_button_GPIO, button2_active, button2_id);        // Initialization: Initialize object, callback function, trigger level, key ID
    button_attach(&button2, BTN_PRESS_REPEAT, on_button2_press_repeat);         // Repeated press event
    button_attach(&button2, BTN_SINGLE_CLICK, on_button2_single_click);         // Single click event
    button_attach(&button2, BTN_DOUBLE_CLICK, on_button2_double_click);         // Double click event
    button_attach(&button2, BTN_LONG_PRESS_START, on_button2_long_press_start); // Long press event
    button_attach(&button2, BTN_PRESS_DOWN, on_button2_press_down);             // Press event
    button_attach(&button2, BTN_PRESS_UP, on_button2_press_up);                 // Release event
    button_attach(&button2, BTN_LONG_PRESS_HOLD, on_button2_long_press_hold);   // Long press hold event

    button_init(&button1, read_button_GPIO, button1_active, button1_id);        // 初始化 初始化对象 回调函数 触发电平 按键ID
    button_attach(&button1, BTN_SINGLE_CLICK, on_button1_single_click);         //Single click event
    button_attach(&button1, BTN_LONG_PRESS_START, on_button1_long_press_start); //Long press event
    button_attach(&button1, BTN_PRESS_REPEAT, on_button1_press_repeat);         //Continuous click event
    button_attach(&button1, BTN_PRESS_UP, on_button1_press_up);                 //Release event

    button_init(&button3, read_button_GPIO, button3_active, button3_id); // Initialization Initialize object Callback function Trigger level Button ID
    button_attach(&button3, BTN_SINGLE_CLICK, on_button3_single_click);  // Click event

    const esp_timer_create_args_t clock_tick_timer_args = {
        .callback = &clock_task_callback,
        .name     = "clock_task",
        .arg      = NULL,
    };
    esp_timer_handle_t clock_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&clock_tick_timer_args, &clock_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(clock_tick_timer, 1000 * 5)); //5ms
    button_start(&button2);                                                // Start button
    button_start(&button1);                                                // Start button
    button_start(&button3);                                                // Start button
}

/*Event function*/
/*Continuous clicking*/
static void on_button2_press_repeat(Button *btn_handle) {
    //xEventGroupSetBits(key_groups,set_bit_button(0));
}

/*Click*/
static void on_button2_single_click(Button *btn_handle) {
    xEventGroupSetBits(key_groups, set_bit_button(0));
}

/*Double-click*/
static void on_button2_double_click(Button *btn_handle) {
    //xEventGroupSetBits(key_groups,set_bit_button(0));
}

/*Long press*/
static void on_button2_long_press_start(Button *btn_handle) {
    xEventGroupSetBits(key_groups, set_bit_button(1));
}

/*Long press to hold*/
static void on_button2_long_press_hold(Button *btn_handle) {
}

/*Press*/
static void on_button2_press_down(Button *btn_handle) {
}

/*Strumming*/
static void on_button2_press_up(Button *btn_handle) {
    xEventGroupSetBits(key_groups, set_bit_button(2));
}

/*boot button*/
/*Click*/
static void on_button1_single_click(Button *btn_handle) {
    xEventGroupSetBits(boot_groups, set_bit_button(0));
}

/*Long press*/
static void on_button1_long_press_start(Button *btn_handle) {
    xEventGroupSetBits(boot_groups, set_bit_button(1));
}

/*Continuous clicking*/
static void on_button1_press_repeat(Button *btn_handle) {
    xEventGroupSetBits(boot_groups, set_bit_button(2));
}

/*Strumming*/
static void on_button1_press_up(Button *btn_handle) {
    xEventGroupSetBits(boot_groups, set_bit_button(3));
}

/*pwr button*/
static void on_button3_single_click(Button *btn_handle) {
    xEventGroupSetBits(pwr_groups, set_bit_button(0));
}

/*Other encapsulated functions*/
uint8_t user_button_get_repeat_count(void) {
    return (button_get_repeat_count(&button2));
}

uint8_t user_boot_get_repeat_count(void) {
    return (button_get_repeat_count(&button1));
}