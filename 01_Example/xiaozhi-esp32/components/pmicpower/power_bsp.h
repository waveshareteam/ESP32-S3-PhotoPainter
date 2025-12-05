#pragma once

#include "i2c_bsp.h"

void Custom_PmicPortInit(I2cMasterBus *i2cbus,uint8_t dev_addr);
void Custom_PmicRegisterInit(void);
void Axp2101_isChargingTask(void *arg);

