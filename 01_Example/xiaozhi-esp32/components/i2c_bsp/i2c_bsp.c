#include "i2c_bsp.h"
#include "freertos/FreeRTOS.h"
#include <stdio.h>

#define AXP2101Addr 0x34
#define RTCAddr 0x51
#define SHTC3Addr 0x70
#define ES8311Addr 0x18
#define ES7210Addr 0x40

#define ESP32_SCL_NUM 48
#define ESP32_SDA_NUM 47

static i2c_master_bus_handle_t user_i2c_handle    = NULL;
i2c_master_dev_handle_t        axp2101_dev_handle = NULL;
i2c_master_dev_handle_t        rtc_dev_handle     = NULL;
i2c_master_dev_handle_t        shtc3_handle       = NULL;
i2c_master_dev_handle_t        es8311_dev_handle  = NULL;
i2c_master_dev_handle_t        es7210_dev_handle  = NULL;

static uint32_t i2c_data_pdMS_TICKS = 0;
static uint32_t i2c_done_pdMS_TICKS = 0;

void i2c_master_Init(void) {
    i2c_data_pdMS_TICKS = pdMS_TO_TICKS(5000);
    i2c_done_pdMS_TICKS = pdMS_TO_TICKS(1000);

    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .i2c_port          = 0,
        .scl_io_num        = ESP32_SCL_NUM,
        .sda_io_num        = ESP32_SDA_NUM,
        .glitch_ignore_cnt = 7,
        .flags             = {
                        .enable_internal_pullup = true,
        },
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &user_i2c_handle));

    /*add dev*/
    i2c_device_config_t dev_cfg =
        {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address  = RTCAddr,
            .scl_speed_hz    = 300000,
        };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(user_i2c_handle, &dev_cfg, &rtc_dev_handle));

    dev_cfg.device_address = SHTC3Addr;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(user_i2c_handle, &dev_cfg, &shtc3_handle));

    dev_cfg.device_address = AXP2101Addr;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(user_i2c_handle, &dev_cfg, &axp2101_dev_handle));

    dev_cfg.device_address = ES8311Addr;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(user_i2c_handle, &dev_cfg, &es8311_dev_handle));

    dev_cfg.device_address = ES7210Addr;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(user_i2c_handle, &dev_cfg, &es7210_dev_handle));
}

uint8_t i2c_write_buff(i2c_master_dev_handle_t dev_handle, int reg, uint8_t *buf, uint8_t len) {
    uint8_t  ret;
    uint8_t *pbuf = NULL;
    ret           = i2c_master_bus_wait_all_done(user_i2c_handle, i2c_done_pdMS_TICKS);
    if (ret != ESP_OK)
        return ret;
    if (reg == -1) {
        ret = i2c_master_transmit(dev_handle, buf, len, i2c_data_pdMS_TICKS);
    } else {
        pbuf    = (uint8_t *) malloc(len + 1);
        pbuf[0] = reg;
        for (uint8_t i = 0; i < len; i++) {
            pbuf[i + 1] = buf[i];
        }
        ret = i2c_master_transmit(dev_handle, pbuf, len + 1, i2c_data_pdMS_TICKS);
        free(pbuf);
        pbuf = NULL;
    }
    return ret;
}

uint8_t i2c_master_write_read_dev(i2c_master_dev_handle_t dev_handle, uint8_t *writeBuf, uint8_t writeLen, uint8_t *readBuf, uint8_t readLen) {
    uint8_t ret;
    ret = i2c_master_bus_wait_all_done(user_i2c_handle, i2c_done_pdMS_TICKS);
    if (ret != ESP_OK)
        return ret;
    ret = i2c_master_transmit_receive(dev_handle, writeBuf, writeLen, readBuf, readLen, i2c_data_pdMS_TICKS);
    return ret;
}

uint8_t i2c_read_buff(i2c_master_dev_handle_t dev_handle, int reg, uint8_t *buf, uint8_t len) {
    uint8_t ret;
    uint8_t addr = 0;
    ret          = i2c_master_bus_wait_all_done(user_i2c_handle, i2c_done_pdMS_TICKS);
    if (ret != ESP_OK)
        return ret;
    if (reg == -1) {
        ret = i2c_master_receive(dev_handle, buf, len, i2c_data_pdMS_TICKS);
    } else {
        addr = (uint8_t) reg;
        ret  = i2c_master_transmit_receive(dev_handle, &addr, 1, buf, len, i2c_data_pdMS_TICKS);
    }
    return ret;
}