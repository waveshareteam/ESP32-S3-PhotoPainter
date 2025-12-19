#pragma once

#include "i2c_bsp.h"

#define AXP2101_iqr_PIN             GPIO_NUM_21
#define AXP2101_CHGLED_PIN          GPIO_NUM_3

void Custom_PmicPortInit(I2cMasterBus *i2cbus,uint8_t dev_addr);
void Custom_PmicRegisterInit(void);
void Axp2101_isChargingTask(void *arg);

