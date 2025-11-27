#include "axp_prot.h"
#include "XPowersLib.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "i2c_bsp.h"
#include <stdio.h>
#include <inttypes.h>

const char *TAG = "axp2101";

#define AXP2101_iqr_PIN GPIO_NUM_21
#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;
static XPowersPMU axp2101;

static int AXP2101_SLAVE_Read(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len) {
    int ret;
    uint8_t count = 5;
    do
    {
        ret = (i2c_read_buff(axp2101_dev_handle, regAddr, data, len) == ESP_OK) ? 0 : -1;
        if (ret == 0)
            break;
        vTaskDelay(pdMS_TO_TICKS(100));
        count--;
    } while (count);
    return ret;
}

static int AXP2101_SLAVE_Write(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len) {
    int ret;
    uint8_t count = 5;
    do
    {
        ret = (i2c_write_buff(axp2101_dev_handle, regAddr, data, len) == ESP_OK) ? 0 : -1;
        if (ret == 0)
            break;
        vTaskDelay(pdMS_TO_TICKS(100));
        count--;
    } while (count);
    return ret;
}

static void IRAM_ATTR GpioIsrFunction(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void axp2101_iqrLoop(void* arg)
{
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            //ESP_LOGW("AXP2101 ISR","GPIO[%"PRIu32"] intr, val: %d", io_num, gpio_get_level((gpio_num_t)io_num));
            vTaskDelay(pdMS_TO_TICKS(5));
            if(!gpio_get_level((gpio_num_t)io_num)) {
                // Get PMU Interrupt Status Register
                uint32_t status = axp2101.getIrqStatus();
                ESP_LOGW("AXP2101 ISR","0x%x", status);

                if (axp2101.isDropWarningLevel2Irq()) {
                    ESP_LOGW("AXP2101 ISR","isDropWarningLevel2");
                }
                if (axp2101.isDropWarningLevel1Irq()) {
                    ESP_LOGW("AXP2101 ISR","isDropWarningLevel1");
                }
                if (axp2101.isGaugeWdtTimeoutIrq()) {
                    ESP_LOGW("AXP2101 ISR","isWdtTimeout");
                }
                if (axp2101.isBatChargerOverTemperatureIrq()) {
                    ESP_LOGW("AXP2101 ISR","isBatChargeOverTemperature");
                }
                if (axp2101.isBatWorkOverTemperatureIrq()) {
                    ESP_LOGW("AXP2101 ISR","isBatWorkOverTemperature");
                }
                if (axp2101.isBatWorkUnderTemperatureIrq()) {
                    ESP_LOGW("AXP2101 ISR","isBatWorkUnderTemperature");
                }
                if (axp2101.isVbusInsertIrq()) {
                    ESP_LOGW("AXP2101 ISR","isVbusInsert");
                }
                if (axp2101.isVbusRemoveIrq()) {
                    ESP_LOGW("AXP2101 ISR","isVbusRemove");
                }
                if (axp2101.isBatInsertIrq()) {
                    ESP_LOGW("AXP2101 ISR","isBatInsert");
                }
                if (axp2101.isBatRemoveIrq()) {
                    ESP_LOGW("AXP2101 ISR","isBatRemove");
                }

                if (axp2101.isPekeyShortPressIrq()) {
                    ESP_LOGW("AXP2101 ISR","isPekeyShortPress");

                    ESP_LOGW("AXP2101 ISR","Read pmu data buffer :");
                    uint8_t data[4] = {0};
                    axp2101.readDataBuffer(data, XPOWERS_AXP2101_DATA_BUFFER_SIZE);
                    ESP_LOGW("AXP2101 ISR","%d,%d,%d,%d",data[0],data[1],data[2],data[3]);
                }

                if (axp2101.isPekeyLongPressIrq()) {
                    ESP_LOGW("AXP2101 ISR","isPekeyLongPress");
                    ESP_LOGW("AXP2101 ISR","write pmu data buffer .");
                    uint8_t data[4] = {1, 2, 3, 4};
                    axp2101.writeDataBuffer(data, XPOWERS_AXP2101_DATA_BUFFER_SIZE);
                }

                if (axp2101.isPekeyNegativeIrq()) {
                    ESP_LOGW("AXP2101 ISR","isPekeyNegative");
                }
                if (axp2101.isPekeyPositiveIrq()) {
                    ESP_LOGW("AXP2101 ISR","isPekeyPositive");
                }
                if (axp2101.isWdtExpireIrq()) {
                    ESP_LOGW("AXP2101 ISR","isWdtExpire");
                }
                if (axp2101.isLdoOverCurrentIrq()) {
                    ESP_LOGW("AXP2101 ISR","isLdoOverCurrentIrq");
                }
                if (axp2101.isBatfetOverCurrentIrq()) {
                    ESP_LOGW("AXP2101 ISR","isBatfetOverCurrentIrq");
                }
                if (axp2101.isBatChargeDoneIrq()) {
                    ESP_LOGW("AXP2101 ISR","isBatChargeDone");
                }
                if (axp2101.isBatChargeStartIrq()) {
                    ESP_LOGW("AXP2101 ISR","isBatChargeStart");
                }
                if (axp2101.isBatDieOverTemperatureIrq()) {
                    ESP_LOGW("AXP2101 ISR","isBatDieOverTemperature");
                }
                if (axp2101.isChargeOverTimeoutIrq()) {
                    ESP_LOGW("AXP2101 ISR","isChargeOverTimeout");
                }
                if (axp2101.isBatOverVoltageIrq()) {
                    ESP_LOGW("AXP2101 ISR","isBatOverVoltage");
                }
                axp2101.clearIrqStatus();
            }
        }
    }
}

static void axp2101_irq_init(void) {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1ULL<<AXP2101_iqr_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en =  GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT));
    ESP_ERROR_CHECK(gpio_isr_handler_add(AXP2101_iqr_PIN, GpioIsrFunction, (void*) AXP2101_iqr_PIN));
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(axp2101_iqrLoop, "axp2101_iqrLoop", 3072, NULL, 10, NULL);
    
}

void axp_i2c_prot_init(void) {
    if (axp2101.begin(AXP2101_SLAVE_ADDRESS, AXP2101_SLAVE_Read, AXP2101_SLAVE_Write)) {
        ESP_LOGI(TAG, "Init PMU SUCCESS!");
    } else {
        ESP_LOGE(TAG, "Init PMU FAILED!");
    }
}

void axp_cmd_init(void) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    int reg1 = axp2101.readRegister(0x20);
    int reg2 = axp2101.readRegister(0x21);
    ESP_LOGE("axp2101_init_log","reg_20:0x%02x,reg_21:0x%02x",reg1,reg2);
    int reg3 = axp2101.readRegister(0x00);
    int reg4 = axp2101.readRegister(0x01);
    ESP_LOGE("axp2101_init_log","reg_00:0x%02x,reg_01:0x%02x",reg3,reg4);
    if(axp2101.getALDO4Voltage() != 3300) {
        axp2101.setALDO4Voltage(3300);
        ESP_LOGW("axp2101_init_log","Set ALDO4 to output 3V3");
    }
    if(axp2101.getALDO3Voltage() != 3300) {
        axp2101.setALDO3Voltage(3300);
        ESP_LOGW("axp2101_init_log","Set ALDO3 to output 3V3");
    }
}

void axp2101_isCharging_task(void *arg) {
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(20000));
        ESP_LOGI(TAG, "isCharging: %s", axp2101.isCharging() ? "YES" : "NO");
        uint8_t charge_status = axp2101.getChargerStatus();
        if (charge_status == XPOWERS_AXP2101_CHG_TRI_STATE) {
            ESP_LOGI(TAG, "Charger Status: tri_charge");
        } else if (charge_status == XPOWERS_AXP2101_CHG_PRE_STATE) {
            ESP_LOGI(TAG, "Charger Status: pre_charge");
        } else if (charge_status == XPOWERS_AXP2101_CHG_CC_STATE) {
            ESP_LOGI(TAG, "Charger Status: constant charge");
        } else if (charge_status == XPOWERS_AXP2101_CHG_CV_STATE) {
            ESP_LOGI(TAG, "Charger Status: constant voltage");
        } else if (charge_status == XPOWERS_AXP2101_CHG_DONE_STATE) {
            ESP_LOGI(TAG, "Charger Status: charge done");
        } else if (charge_status == XPOWERS_AXP2101_CHG_STOP_STATE) {
            ESP_LOGI(TAG, "Charger Status: not charge");
        }
        ESP_LOGI(TAG, "getBattVoltage: %d mV", axp2101.getBattVoltage());
    }
}

