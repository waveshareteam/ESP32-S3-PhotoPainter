#include "bsp_config.h"
#include "bsp_fac.h"
#include "src/button/button.h"

I2cMasterBus I2cbus(48, 47, 0);
ePaperPort EPDPort(EPD_MOSI_PIN, EPD_SCK_PIN, EPD_DC_PIN, EPD_CS_PIN, EPD_RST_PIN, EPD_BUSY_PIN, LCD_WIDTH, LCD_HEIGHT);

static EventGroupHandle_t led_groups;

void axp2101_irq_init(void) {
  gpio_config_t gpio_conf = {};
  gpio_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_conf.mode = GPIO_MODE_OUTPUT;
  gpio_conf.pin_bit_mask = ((uint64_t)0x01 << AXP2101_iqr_PIN);
  gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;

  ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
  gpio_set_level(AXP2101_iqr_PIN, 0);
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level(AXP2101_iqr_PIN, 1);
  vTaskDelay(pdMS_TO_TICKS(200));
}

void GoSLeep(void) {
  esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_AUTO);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  const uint64_t ext_wakeup_pin_1_mask = 1ULL << GPIO_NUM_0;
  ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup_io(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ANY_LOW));  //使能BOOT 低电平唤醒
  axp_basic_sleep_start();
  esp_deep_sleep_start();  // esp32进入低功耗
}

static void Boot_ButtonLoopTask(void *arg) {
  uint8_t flags = 1;
  for (;;) {
    EventBits_t even = xEventGroupWaitBits(boot_groups, set_bit_all, pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
    if (get_bit_button(even, 0)) {
      if (flags) {
        flags = 0;
        xEventGroupSetBits(led_groups, 0x01);
        EPDPort.EPD_SDcardBmpShakingColor("/sdcard/06_user_foundation_img/4.bmp");
        EPDPort.EPD_Display(EPDPort.EPD_GetIMGBuffer());
        xEventGroupClearBits(led_groups, 0x01);
        flags = 1;
      }
    }
  }
}

static void PWR_ButtonLoopTask(void *arg) {
  for (;;) {
    EventBits_t even = xEventGroupWaitBits(pwr_groups, set_bit_all, pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
    if (get_bit_button(even, 0)) {
      GoSLeep();
    }
  }
}

static void Led_LoopTask(void *arg) {
  for (;;) {
    EventBits_t even = xEventGroupWaitBits(led_groups, 0x01, pdFALSE, pdFALSE, 1000);
    if (even & 0x01) {
      Led_set(LED_PIN_Green, LED_ON);
      vTaskDelay(pdMS_TO_TICKS(100));
      Led_set(LED_PIN_Green, LED_OFF);
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

void setup() {
  Serial.begin(115200);
  axp2101_irq_init();
  vTaskDelay(pdMS_TO_TICKS(2000));
  led_groups = xEventGroupCreate();
  Custom_PmicPortInit(&I2cbus, 0x34);
  Custom_PmicRegisterInit();
  Led_Init();
  Led_set(LED_PIN_Red, LED_ON);
  button_Init();
  Custom_SDcardInit();
  EPDPort.EPD_Init();
  xTaskCreate(Boot_ButtonLoopTask, "Boot_ButtonLoopTask", 6 * 1024, NULL, 3, NULL);
  xTaskCreate(PWR_ButtonLoopTask, "PWR_ButtonLoopTask", 4 * 1024, NULL, 3, NULL);
  xTaskCreate(Led_LoopTask, "Led_LoopTask", 4 * 1024, NULL, 2, NULL);
}

void loop() {
}
