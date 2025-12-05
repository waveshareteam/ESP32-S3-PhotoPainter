#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include "display_bsp.h"

ePaperPort::ePaperPort(int mosi, int scl, int dc, int cs, int rst, int busy, int width, int height, spi_host_device_t spihost) : mosi_(mosi), scl_(scl), dc_(dc), cs_(cs), rst_(rst), busy_(busy), width_(width), height_(height) {
    esp_err_t        ret;
    spi_bus_config_t buscfg   = {};
    int              transfer = width_ * height_;
    DisplayLen                = transfer / 2; //(1byte 2ipex)
    DispBuffer                = (uint8_t *) heap_caps_malloc(DisplayLen, MALLOC_CAP_SPIRAM);
    assert(DispBuffer);
    RotationBuffer             = (uint8_t *) heap_caps_malloc(DisplayLen, MALLOC_CAP_SPIRAM);
    assert(RotationBuffer);
    BmpSrcBuffer              = (uint8_t *) heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM); 
    assert(BmpSrcBuffer);
    buscfg.miso_io_num                   = -1;
    buscfg.mosi_io_num                   = mosi;
    buscfg.sclk_io_num                   = scl;
    buscfg.quadwp_io_num                 = -1;
    buscfg.quadhd_io_num                 = -1;
    buscfg.max_transfer_sz               = transfer;
    spi_device_interface_config_t devcfg = {};
    devcfg.spics_io_num                  = -1;
    devcfg.clock_speed_hz                = 10 * 1000 * 1000;    // Clock out at 40 MHz
    devcfg.mode                          = 0;                   // SPI mode 0
    devcfg.queue_size                    = 7;                   // We want to be able to queue 7 transactions at a time
    devcfg.flags                         = SPI_DEVICE_HALFDUPLEX;
    ret                                  = spi_bus_initialize(spihost, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    ret = spi_bus_add_device(spihost, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type     = GPIO_INTR_DISABLE;
    gpio_conf.mode          = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask  = (0x1ULL << rst_) | (0x1ULL << dc_) | (0x1ULL << cs_);
    gpio_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en    = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));

    gpio_conf.intr_type    = GPIO_INTR_DISABLE;
    gpio_conf.mode         = GPIO_MODE_INPUT;
    gpio_conf.pin_bit_mask = (0x1ULL << busy_);
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));

    Set_ResetIOLevel(1);
}

ePaperPort::~ePaperPort() {
}

void ePaperPort::Set_ResetIOLevel(uint8_t level) {
    gpio_set_level((gpio_num_t) rst_, level ? 1 : 0);
}

void ePaperPort::Set_CSIOLevel(uint8_t level) {
    gpio_set_level((gpio_num_t) cs_, level ? 1 : 0);
}

void ePaperPort::Set_DCIOLevel(uint8_t level) {
    gpio_set_level((gpio_num_t) dc_, level ? 1 : 0);
}

uint8_t ePaperPort::Get_BusyIOLevel() {
    return gpio_get_level((gpio_num_t) busy_);
}

void ePaperPort::EPD_Reset(void) {
    Set_ResetIOLevel(1);
    vTaskDelay(pdMS_TO_TICKS(50));
    Set_ResetIOLevel(0);
    vTaskDelay(pdMS_TO_TICKS(20));
    Set_ResetIOLevel(1);
    vTaskDelay(pdMS_TO_TICKS(50));
}

void ePaperPort::EPD_LoopBusy(void) {
    while (1) {
        if (Get_BusyIOLevel()) {
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void ePaperPort::SPI_Write(uint8_t data) {
    esp_err_t         ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length    = 8;
    t.tx_buffer = &data;
    ret         = spi_device_polling_transmit(spi, &t);
    assert(ret == ESP_OK);
}

void ePaperPort::EPD_SendCommand(uint8_t Reg) {
    Set_DCIOLevel(0);
    Set_CSIOLevel(0);
    SPI_Write(Reg);
    Set_CSIOLevel(1);
}

void ePaperPort::EPD_SendData(uint8_t Data) {
    Set_DCIOLevel(1);
    Set_CSIOLevel(0);
    SPI_Write(Data);
    Set_CSIOLevel(1);
}

void ePaperPort::EPD_Sendbuffera(uint8_t *Data, int len) {
    Set_DCIOLevel(1);
    Set_CSIOLevel(0);
    esp_err_t         ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    int      len_scl = len / 5000;
    int      len_dcl = len % 5000;
    uint8_t *ptr     = Data;
    while (len_scl) {
        t.length    = 8 * 5000;
        t.tx_buffer = ptr;
        ret         = spi_device_polling_transmit(spi, &t);
        assert(ret == ESP_OK);
        len_scl--;
        ptr += 5000;
    }
    t.length    = 8 * len_dcl;
    t.tx_buffer = ptr;
    ret         = spi_device_polling_transmit(spi, &t);
    assert(ret == ESP_OK);
    Set_CSIOLevel(1);
}

void ePaperPort::EPD_TurnOnDisplay(void) {

    EPD_SendCommand(0x04); // POWER_ON
    EPD_LoopBusy();

    // Second setting
    EPD_SendCommand(0x06);
    EPD_SendData(0x6F);
    EPD_SendData(0x1F);
    EPD_SendData(0x17);
    EPD_SendData(0x49);

    EPD_SendCommand(0x12); // DISPLAY_REFRESH
    EPD_SendData(0x00);
    EPD_LoopBusy();

    EPD_SendCommand(0x02); // POWER_OFF
    EPD_SendData(0X00);
    EPD_LoopBusy();
}

void ePaperPort::Set_Rotation(uint8_t rot) {
    Rotation = rot;
}

void ePaperPort::Set_Mirror(uint8_t mirr_x,uint8_t mirr_y) {
    mirrx = mirr_x;
    mirry = mirr_y;
}

void ePaperPort::EPD_Init() {
    EPD_Reset();
    EPD_LoopBusy();
    vTaskDelay(pdMS_TO_TICKS(50));

    EPD_SendCommand(0xAA);
    EPD_SendData(0x49);
    EPD_SendData(0x55);
    EPD_SendData(0x20);
    EPD_SendData(0x08);
    EPD_SendData(0x09);
    EPD_SendData(0x18);

    EPD_SendCommand(0x01);
    EPD_SendData(0x3F);

    EPD_SendCommand(0x00);
    EPD_SendData(0x5F);
    EPD_SendData(0x69);

    EPD_SendCommand(0x03);
    EPD_SendData(0x00);
    EPD_SendData(0x54);
    EPD_SendData(0x00);
    EPD_SendData(0x44);

    EPD_SendCommand(0x05);
    EPD_SendData(0x40);
    EPD_SendData(0x1F);
    EPD_SendData(0x1F);
    EPD_SendData(0x2C);

    EPD_SendCommand(0x06);
    EPD_SendData(0x6F);
    EPD_SendData(0x1F);
    EPD_SendData(0x17);
    EPD_SendData(0x49);

    EPD_SendCommand(0x08);
    EPD_SendData(0x6F);
    EPD_SendData(0x1F);
    EPD_SendData(0x1F);
    EPD_SendData(0x22);

    EPD_SendCommand(0x30);
    EPD_SendData(0x03);

    EPD_SendCommand(0x50);
    EPD_SendData(0x3F);

    EPD_SendCommand(0x60);
    EPD_SendData(0x02);
    EPD_SendData(0x00);

    EPD_SendCommand(0x61);
    EPD_SendData(0x03);
    EPD_SendData(0x20);
    EPD_SendData(0x01);
    EPD_SendData(0xE0);

    EPD_SendCommand(0x84);
    EPD_SendData(0x01);

    EPD_SendCommand(0xE3);
    EPD_SendData(0x2F);

    EPD_SendCommand(0x04);
    EPD_LoopBusy();
    EPD_DispClear(ColorWhite);
}

void ePaperPort::EPD_DispClear(uint8_t color) {
    uint8_t *buffer = DispBuffer;
    for (int j = 0; j < DisplayLen; j++) {
        buffer[j] = (color << 4) | color;
    }
}

void ePaperPort::EPD_Display() {
    EPD_PixelRotate();
    EPD_SendCommand(0x10);
    EPD_Sendbuffera(RotationBuffer, DisplayLen);
    EPD_TurnOnDisplay();
}

uint8_t* ePaperPort::EPD_GetIMGBuffer() {
    return DispBuffer;
}

void ePaperPort::EPD_SetPixel(uint16_t x, uint16_t y, uint16_t color) {
    if(x >= 800 || y >= 480) {
        ESP_LOGE("Pixel","Beyond the limit: (%d,%d)",x,y);
        return;
    }
    uint32_t index = (y << 8) + (y << 7) + (y << 4) + (x >> 1);
    uint8_t px = DispBuffer[index];
  
    uint8_t xor_mask = (x & 1) ? 0xF0 : 0x0F;
    uint8_t shift    = (x & 1) ? 0     : 4;

    DispBuffer[index] = (px & xor_mask) | (color << shift);
}

uint8_t* ePaperPort::EPD_ParseBMPImage(const char *path) {
    FILE *fp;
    if ((fp = fopen(path, "rb")) == NULL) {
        ESP_LOGE(TAG, "Cann't open the file!");
        return NULL;
    }
    BMPFILEHEADER bmpFileHeader;
    BMPINFOHEADER bmpInfoHeader;

    fseek(fp, 0, SEEK_SET);
    fread(&bmpFileHeader, sizeof(BMPFILEHEADER), 1, fp);  
    fread(&bmpInfoHeader, sizeof(BMPINFOHEADER), 1, fp);

    //ESP_LOGW(TAG, "BMPFILEHEADER:%d,BMPINFOHEADER:%d", sizeof(BMPFILEHEADER),sizeof(BMPINFOHEADER));
    ESP_LOGW(TAG, "(WIDTH:HEIGHT) = (%ld:%ld)", bmpInfoHeader.biWidth, bmpInfoHeader.biHeight);
    //if (bmpInfoHeader.biWidth * bmpInfoHeader.biHeight != 384000) {
    //    ESP_LOGE(TAG, "Incorrect resolution");
    //    fclose(fp);
    //    return NULL;
    //}
    src_width  = bmpInfoHeader.biWidth;
    src_height = bmpInfoHeader.biHeight;
    int readbyte = bmpInfoHeader.biBitCount;
    if (readbyte != 24) {
        ESP_LOGE(TAG, "Bmp image is not 24 bitmap!");
        fclose(fp);
        return NULL;
    }

    fseek(fp, bmpFileHeader.bOffset, SEEK_SET);
    int rowBytes = src_width * 3;
    for (int y = src_height - 1; y >= 0; y--) {
        uint8_t *rowPtr = BmpSrcBuffer + y * rowBytes;
        fread(rowPtr, 1, rowBytes, fp);
    }
    fclose(fp);
    if(src_width == 480)
    {Rotation = 1;src_width = 800;src_height = 480;}
    else 
    {Rotation = 2;}
    return BmpSrcBuffer;
}

uint8_t ePaperPort::EPD_ColorToePaperColor(uint8_t b,uint8_t g,uint8_t r) {
    if(b == 0xff && g == 0xff && r == 0xff) {
        return ColorWhite;
    }
    if(b == 0x0 && g == 0x0 && r == 0x0) {
        return ColorBlack;
    }
    if(b == 0x0 && g == 0x0 && r == 0xff) {
        return ColorRed;
    }
    if(b == 0xff && g == 0x0 && r == 0x0) {
        return ColorBlue;
    }
    if(b == 0x0 && g == 0xff && r == 0x0) {
        return ColorGreen;
    }
    if(b == 0x0 && g == 0xff && r == 0xff) {
        return ColorYellow;
    }
    return ColorWhite;
}

void ePaperPort::EPD_SDcardBmpShakingColor(const char *path,uint16_t x_start, uint16_t y_start) {
    uint8_t r,g,b;
    uint8_t *buffer = EPD_ParseBMPImage(path);
    if(NULL == buffer) {
        return;
    }
    uint8_t* scapeBuffer = (uint8_t*)buffer;
    for(int y = 0; y < src_height; y++) {
        for(int x = 0; x < src_width; x++) {
            int idx = (y * src_width + x) * 3;
            b = scapeBuffer[idx + 0];
            g = scapeBuffer[idx + 1];
            r = scapeBuffer[idx + 2];
            uint8_t color = EPD_ColorToePaperColor(b, g, r);
            EPD_SetPixel(x_start + x, y_start + y, color);
        }
    }
}

void ePaperPort::EPD_DrawStringCN(uint16_t Xstart, uint16_t Ystart, const char * pString, cFONT* font,uint16_t Color_Foreground, uint16_t Color_Background) {
    const char* p_text = pString;
    int x = Xstart, y = Ystart;
    int i, j,Num;
    uint8_t FONT_BACKGROUND = 0xff;
    /* Send the string character by character on EPD */
    while (*p_text != 0) {
        if(*p_text <= 0xE0) {  //ASCII < 126
            for(Num = 0; Num < font->size; Num++) {
                if(*p_text== font->table[Num].index[0]) {
                    const char* ptr = &font->table[Num].matrix[0];
                    for (j = 0; j < font->Height; j++) {
                        for (i = 0; i < font->Width; i++) {
                            if (FONT_BACKGROUND == Color_Background) { //this process is to speed up the scan
                                if (*ptr & (0x80 >> (i % 8))) {
                                    EPD_SetPixel(x + i, y + j, Color_Foreground);
                                }
                            } else {
                                if (*ptr & (0x80 >> (i % 8))) {
                                    EPD_SetPixel(x + i, y + j, Color_Foreground);
                                } else {
                                    EPD_SetPixel(x + i, y + j, Color_Background);
                                }
                            }
                            if (i % 8 == 7) {
                                ptr++;
                            }
                        }
                        if (font->Width % 8 != 0) {
                            ptr++;
                        }
                    }
                    break;
                }
            }
            /* Point on the next character */
            p_text += 1;
            /* Decrement the column position by 16 */
            x += font->ASCII_Width;
        } else {        //Chinese
            for(Num = 0; Num < font->size; Num++) {
                if((*p_text== font->table[Num].index[0]) && (*(p_text+1) == font->table[Num].index[1])  && (*(p_text+2) == font->table[Num].index[2])) {
                    const char* ptr = &font->table[Num].matrix[0];

                    for (j = 0; j < font->Height; j++) {
                        for (i = 0; i < font->Width; i++) {
                            if (FONT_BACKGROUND == Color_Background) { //this process is to speed up the scan
                                if (*ptr & (0x80 >> (i % 8))) {
                                    EPD_SetPixel(x + i, y + j, Color_Foreground);
                                }
                            } else {
                                if (*ptr & (0x80 >> (i % 8))) {
                                    EPD_SetPixel(x + i, y + j, Color_Foreground);
                                } else {
                                    EPD_SetPixel(x + i, y + j, Color_Background);
                                }
                            }
                            if (i % 8 == 7) {
                                ptr++;
                            }
                        }
                        if (font->Width % 8 != 0) {
                            ptr++;
                        }
                    }
                    break;
                }
            }
            /* Point on the next character */
            p_text += 3;
            /* Decrement the column position by 16 */
            x += font->Width;
        }
    }
}

uint8_t ePaperPort::EPD_GetPixel4(const uint8_t* buf, int width, int x, int y) {
    int index = y * (width >> 1) + (x >> 1);
    uint8_t byte = buf[index];
    return (x & 1) ? (byte & 0x0F) : (byte >> 4);
}

void ePaperPort::EPD_SetPixel4(uint8_t* buf, int width, int x, int y, uint8_t px) {
    int index = y * (width >> 1) + (x >> 1);
    uint8_t old = buf[index];
    if (x & 1)
        buf[index] = (old & 0xF0) | (px & 0x0F);
    else
        buf[index] = (old & 0x0F) | (px << 4);
}

void ePaperPort::EPD_PixelRotate() {
    if(Rotation == 3) {
        EPD_Rotate90CCW_Fast(DispBuffer,RotationBuffer,480,800);
    } else if(Rotation == 1) {
        EPD_Rotate90CW_Fast(DispBuffer,RotationBuffer,480,800);
    } else if(Rotation == 2) {
        EPD_Rotate180_Fast(DispBuffer,RotationBuffer,800,480);
    } else {
        memcpy(RotationBuffer, DispBuffer, DisplayLen);
    }
}

void ePaperPort::EPD_Rotate180_Fast(const uint8_t* src, uint8_t* dst, int width, int height)
{
    const int bytesPerRow = width >> 1;
    const int totalRows   = height;    
    for (int y = 0; y < totalRows; y++) {
        const uint8_t* srcRow = src + y * bytesPerRow;
        uint8_t* dstRow = dst + (totalRows - 1 - y) * bytesPerRow;
        for (int x = 0; x < bytesPerRow; x++) {
            uint8_t b = srcRow[x];
            b = (b << 4) | (b >> 4);
            dstRow[bytesPerRow - 1 - x] = b;
        }
    }
}

void ePaperPort::EPD_Rotate90CCW_Fast(const uint8_t* src, uint8_t* dst, int width, int height)
{
    const int srcBytesPerRow = width >> 1;
    for (int y = 0; y < height; y++) {
        const uint8_t* srcRow = src + y * srcBytesPerRow;
        for (int x = 0; x < width; x += 2) {

            uint8_t b = srcRow[x >> 1];
            uint8_t p0 = b >> 4;
            uint8_t p1 = b & 0x0F;
            int ny0 = width - 1 - x;
            int nx0 = y;
            int ny1 = width - 2 - x;
            int nx1 = y;
            EPD_SetPixel4(dst, height, nx0, ny0, p0);
            EPD_SetPixel4(dst, height, nx1, ny1, p1);
        }
    }
}

void ePaperPort::EPD_Rotate90CW_Fast(const uint8_t* src, uint8_t* dst, int width, int height)
{
    const int srcBytesPerRow = width >> 1;
    for (int y = 0; y < height; y++) {
        const uint8_t* srcRow = src + y * srcBytesPerRow;
        for (int x = 0; x < width; x += 2) {

            uint8_t b = srcRow[x >> 1];
            uint8_t p0 = b >> 4;  
            uint8_t p1 = b & 0x0F;
            int ny0 = x;
            int nx0 = height - 1 - y;
            int ny1 = x + 1;
            int nx1 = height - 1 - y;
            EPD_SetPixel4(dst, height, nx0, ny0, p0);
            EPD_SetPixel4(dst, height, nx1, ny1, p1);
        }
    }
}