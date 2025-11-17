#include <stdio.h>
#include <string.h>
#include "epaper_port.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#define EPD_DC_PIN 8
#define EPD_CS_PIN 9
#define EPD_SCK_PIN 10
#define EPD_MOSI_PIN 11
#define EPD_RST_PIN 12
#define EPD_BUSY_PIN 13

#define epaper_rst_1 gpio_set_level(EPD_RST_PIN, 1)
#define epaper_rst_0 gpio_set_level(EPD_RST_PIN, 0)
#define epaper_cs_1 gpio_set_level(EPD_CS_PIN, 1)
#define epaper_cs_0 gpio_set_level(EPD_CS_PIN, 0)
#define epaper_dc_1 gpio_set_level(EPD_DC_PIN, 1)
#define epaper_dc_0 gpio_set_level(EPD_DC_PIN, 0)
#define ReadBusy gpio_get_level(EPD_BUSY_PIN)

static spi_device_handle_t spi;
static void                spi_send_byte(uint8_t cmd);

static void epaper_gpio_init(void) {
    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type     = GPIO_INTR_DISABLE;
    gpio_conf.mode          = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask  = ((uint64_t) 0x01 << EPD_RST_PIN) | ((uint64_t) 0x01 << EPD_DC_PIN) | ((uint64_t) 0x01 << EPD_CS_PIN);
    gpio_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en    = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));

    gpio_conf.intr_type    = GPIO_INTR_DISABLE;
    gpio_conf.mode         = GPIO_MODE_INPUT;
    gpio_conf.pin_bit_mask = ((uint64_t) 0x01 << EPD_BUSY_PIN);
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en   = GPIO_PULLDOWN_ENABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));

    epaper_rst_1;
}

/*Pinout of the 7.3-inch e-Paper module by Waveshare Electronics*/

/*
  Ink screen reset
*/
static void epaper_reset(void) {
    epaper_rst_1;
    vTaskDelay(pdMS_TO_TICKS(50));
    epaper_rst_0;
    vTaskDelay(pdMS_TO_TICKS(20));
    epaper_rst_1;
    vTaskDelay(pdMS_TO_TICKS(50));
}

static void epaper_spi_init(void) {
    esp_err_t        ret;
    spi_bus_config_t buscfg              = {};
    buscfg.miso_io_num                   = -1;
    buscfg.mosi_io_num                   = EPD_MOSI_PIN;
    buscfg.sclk_io_num                   = EPD_SCK_PIN;
    buscfg.quadwp_io_num                 = -1;
    buscfg.quadhd_io_num                 = -1;
    buscfg.max_transfer_sz               = EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT;
    spi_device_interface_config_t devcfg = {};
    devcfg.spics_io_num                  = -1;
    devcfg.clock_speed_hz                = 10 * 1000 * 1000; //Clock out at 10 MHz
    devcfg.mode                          = 0;                //SPI mode 0
    devcfg.queue_size                    = 7;                //We want to be able to queue 7 transactions at a time
    devcfg.flags                         = SPI_DEVICE_HALFDUPLEX;
    ret                                  = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    ret = spi_bus_add_device(SPI3_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
}

/*
  Waiting for the idle signal
*/
static void epaper_readbusyh(void) {
    while (1) {
        if (ReadBusy) {
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/*
Send byte command
*/
static void epaper_SendCommand(uint8_t Reg) {
    epaper_dc_0;
    epaper_cs_0;
    spi_send_byte(Reg);
    epaper_cs_1;
}

/*
Send byte data
*/
void epaper_SendData(uint8_t Data) {
    epaper_dc_1;
    epaper_cs_0;
    spi_send_byte(Data);
    epaper_cs_1;
}

/*send bytes data*/
void epaper_Sendbuffera(uint8_t *Data, int len) {
    epaper_dc_1;
    epaper_cs_0;
    esp_err_t         ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    int      len_scl = len / 5000; //每次发送5000
    int      len_dcl = len % 5000;
    uint8_t *ptr     = Data;
    while (len_scl) {
        t.length    = 8 * 5000;
        t.tx_buffer = ptr;
        ret         = spi_device_polling_transmit(spi, &t); //Transmit!
        assert(ret == ESP_OK);                              //Should have had no issues
        len_scl--;
        ptr += 5000;
    }
    t.length    = 8 * len_dcl;
    t.tx_buffer = ptr;
    ret         = spi_device_polling_transmit(spi, &t); //Transmit!
    assert(ret == ESP_OK);                              //Should have had no issues.
    epaper_cs_1;
}

/*
  The data has been uploaded to Buff
*/
static void epaper_TurnOnDisplay(void) {

    epaper_SendCommand(0x04); // POWER_ON
    epaper_readbusyh();

    //Second setting
    epaper_SendCommand(0x06);
    epaper_SendData(0x6F);
    epaper_SendData(0x1F);
    epaper_SendData(0x17);
    epaper_SendData(0x49);

    epaper_SendCommand(0x12); // DISPLAY_REFRESH
    epaper_SendData(0x00);
    epaper_readbusyh();

    epaper_SendCommand(0x02); // POWER_OFF
    epaper_SendData(0X00);
    epaper_readbusyh();
}

/*
epaper init
*/
void epaper_port_init(void) {
    epaper_spi_init();  //spi init
    epaper_gpio_init(); //ws gpio init
    epaper_reset();     //reset
    epaper_readbusyh();
    vTaskDelay(pdMS_TO_TICKS(50));

    epaper_SendCommand(0xAA); // CMDH
    epaper_SendData(0x49);
    epaper_SendData(0x55);
    epaper_SendData(0x20);
    epaper_SendData(0x08);
    epaper_SendData(0x09);
    epaper_SendData(0x18);

    epaper_SendCommand(0x01); //
    epaper_SendData(0x3F);

    epaper_SendCommand(0x00);
    epaper_SendData(0x5F);
    epaper_SendData(0x69);

    epaper_SendCommand(0x03);
    epaper_SendData(0x00);
    epaper_SendData(0x54);
    epaper_SendData(0x00);
    epaper_SendData(0x44);

    epaper_SendCommand(0x05);
    epaper_SendData(0x40);
    epaper_SendData(0x1F);
    epaper_SendData(0x1F);
    epaper_SendData(0x2C);

    epaper_SendCommand(0x06);
    epaper_SendData(0x6F);
    epaper_SendData(0x1F);
    epaper_SendData(0x17);
    epaper_SendData(0x49);

    epaper_SendCommand(0x08);
    epaper_SendData(0x6F);
    epaper_SendData(0x1F);
    epaper_SendData(0x1F);
    epaper_SendData(0x22);

    epaper_SendCommand(0x30);
    epaper_SendData(0x03);

    epaper_SendCommand(0x50);
    epaper_SendData(0x3F);

    epaper_SendCommand(0x60);
    epaper_SendData(0x02);
    epaper_SendData(0x00);

    epaper_SendCommand(0x61);
    epaper_SendData(0x03);
    epaper_SendData(0x20);
    epaper_SendData(0x01);
    epaper_SendData(0xE0);

    epaper_SendCommand(0x84);
    epaper_SendData(0x01);

    epaper_SendCommand(0xE3);
    epaper_SendData(0x2F);

    epaper_SendCommand(0x04); //PWR on
    epaper_readbusyh();       //waiting for the electronic paper IC to release the idle signal
}

/*
spi send byte data
*/

static void spi_send_byte(uint8_t cmd) {
    esp_err_t         ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length    = 8;
    t.tx_buffer = &cmd;
    ret         = spi_device_polling_transmit(spi, &t); //Transmit!
    assert(ret == ESP_OK);                              //Should have had no issues.
}

/*Shared function API*/

/*
clear screen
*/
void epaper_port_clear(uint8_t *Image, uint8_t color) {
    uint16_t Width, Height;
    Width  = (EXAMPLE_LCD_WIDTH % 2 == 0) ? (EXAMPLE_LCD_WIDTH / 2) : (EXAMPLE_LCD_WIDTH / 2 + 1);
    Height = EXAMPLE_LCD_HEIGHT;

    epaper_SendCommand(0x10);
    for (int j = 0; j < Height * Width; j++) {
        Image[j] = (color << 4) | color;
    }
    epaper_Sendbuffera(Image, Height * Width);
    epaper_TurnOnDisplay();
}

/*
display
*/
void epaper_port_display(uint8_t *Image) {
    uint16_t Width, Height;
    Width  = (EXAMPLE_LCD_WIDTH % 2 == 0) ? (EXAMPLE_LCD_WIDTH / 2) : (EXAMPLE_LCD_WIDTH / 2 + 1);
    Height = EXAMPLE_LCD_HEIGHT;

    epaper_SendCommand(0x10);
    epaper_Sendbuffera(Image, Height * Width);
    epaper_TurnOnDisplay();
}