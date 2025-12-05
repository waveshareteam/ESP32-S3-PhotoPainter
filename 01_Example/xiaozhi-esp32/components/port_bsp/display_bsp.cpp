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
    DispBuffer180             = (uint8_t *) heap_caps_malloc(DisplayLen, MALLOC_CAP_SPIRAM);
    assert(DispBuffer180);
    BmpSrcBuffer              = (uint8_t *) heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM); 
    assert(BmpSrcBuffer);
    QueueCacheBuffer          = (uint8_t *) heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM);
    assert(QueueCacheBuffer);
    buscfg.miso_io_num                   = -1;
    buscfg.mosi_io_num                   = mosi;
    buscfg.sclk_io_num                   = scl;
    buscfg.quadwp_io_num                 = -1;
    buscfg.quadhd_io_num                 = -1;
    buscfg.max_transfer_sz               = transfer;
    spi_device_interface_config_t devcfg = {};
    devcfg.spics_io_num                  = -1;
    devcfg.clock_speed_hz                = 10 * 1000 * 1000; // Clock out at 40 MHz
    devcfg.mode                          = 0;                // SPI mode 0
    devcfg.queue_size                    = 7;                // We want to be able to queue 7 transactions at a time
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

void ePaperPort::EPD_DispClear(uint8_t *Image, uint8_t color) {
    uint8_t *buffer = (Image == NULL) ? DispBuffer : Image;
    EPD_SendCommand(0x10);
    for (int j = 0; j < DisplayLen; j++) {
        buffer[j] = (color << 4) | color;
    }
    EPD_Sendbuffera(buffer, DisplayLen);
    EPD_TurnOnDisplay();
}

void ePaperPort::EPD_DispClear(uint8_t color) {
    uint8_t *buffer = DispBuffer;
    EPD_SendCommand(0x10);
    for (int j = 0; j < DisplayLen; j++) {
        buffer[j] = (color << 4) | color;
    }
}

void ePaperPort::EPD_Display(uint8_t *Image) {
    uint8_t *buffer = (Image == NULL) ? DispBuffer : Image;
    EPD_SendCommand(0x10);
    EPD_Sendbuffera(buffer, DisplayLen);
    EPD_TurnOnDisplay();
}

void ePaperPort::EPD_Display() {
    PixelRotate180(DispBuffer,DispBuffer180);
    EPD_SendCommand(0x10);
    EPD_Sendbuffera(DispBuffer180, DisplayLen);
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
    if(src_width == 800)
    {Rotation = 2;}
    else if(src_width == 480)
    {Rotation = 1;}
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

void ePaperPort::EPD_SDcardBmpShakingColor(const char *path) {
    uint8_t r,g,b;
    uint8_t *buffer = EPD_ParseBMPImage(path);
    if(NULL == buffer) {
        return;
    }
    if(Rotation == 1) {
        RotateBMP24bit(BmpSrcBuffer,QueueCacheBuffer,480,800,1);
    } else if(Rotation == 2) {
        RotateBMP24bit(BmpSrcBuffer,QueueCacheBuffer,480,800,2);
    }
    LandscapeBuffer = (uint8_t (*)[800][3])QueueCacheBuffer;
    for(int y = 0; y < height_; y++) {
        for(int x = 0; x < width_; x++) { 
            b = LandscapeBuffer[y][x][0];
            g = LandscapeBuffer[y][x][1];
            r = LandscapeBuffer[y][x][2];
            uint8_t color = EPD_ColorToePaperColor(b,g,r);
            EPD_SetPixel(x,y,color);
        }
    }
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

void ePaperPort::RotateBMP24bit(const uint8_t *src, uint8_t* dst, int in_width, int in_height, int rotation)
{
    int out_width  = (rotation == 1 || rotation == 3) ? in_height : in_width;
    int out_height = (rotation == 1 || rotation == 3) ? in_width  : in_height;

    if (!dst) return;

    for (int y = 0; y < in_height; y++)
    {
        for (int x = 0; x < in_width; x++)
        {
            const uint8_t *pSrc = src + (y * in_width + x) * 3;
            uint8_t *pDst = NULL;

            switch(rotation)
            {
                case 0: 
                    pDst = dst + (y * out_width + x) * 3;
                    break;
                case 1:
                    pDst = dst + (x * out_width + (out_width - 1 - y)) * 3;
                    break;
                case 2:
                    pDst = dst + ((out_height - 1 - y) * out_width + (out_width - 1 - x)) * 3;
                    break;
                case 3: 
                    pDst = dst + ((out_height - 1 - x) * out_width + y) * 3;
                    break;
                default: 
                    pDst = dst + (y * out_width + x) * 3;
                    break;
            }
            pDst[0] = pSrc[0];
            pDst[1] = pSrc[1];
            pDst[2] = pSrc[2];
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

void ePaperPort::PixelRotate180(const uint8_t* src, uint8_t* dst)
{
    const int width = 800;
    const int height = 480;
    const int bytesPerRow = width / 2;

    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {

            // 源像素 index
            int srcIndex = y * bytesPerRow + (x >> 1);
            uint8_t srcByte = src[srcIndex];
            uint8_t srcPixel = (x & 1) ? (srcByte & 0x0F) : (srcByte >> 4);

            // 旋转后的坐标
            int rx = width  - 1 - x;  // 799-x
            int ry = height - 1 - y;  // 479-y

            // 目标像素 index
            int dstIndex = ry * bytesPerRow + (rx >> 1);
            uint8_t px = dst[dstIndex];

            if (rx & 1)
                dst[dstIndex] = (px & 0xF0) | (srcPixel & 0x0F);
            else
                dst[dstIndex] = (px & 0x0F) | (srcPixel << 4);
        }
    }
}


//void ePaperPort::ConvertBufferTo3DArray_Fast(uint8_t *buffer, uint8_t a[][800][3]) { 
//    int rowBytes = width_ * 3; 
//    for (int y = 0; y < height_; y++) { 
//        memcpy(a[y], buffer + y * rowBytes, rowBytes); 
//    } 
//}
//