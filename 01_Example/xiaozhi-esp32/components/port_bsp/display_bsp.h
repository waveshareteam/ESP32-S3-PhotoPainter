#pragma once

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include "fonts.h"

enum ColorSelection {
    ColorBlack = 0,    
    ColorWhite,
    ColorYellow,
    ColorRed,
    ColorBlue = 5,
    ColorGreen
};

/*Bitmap file header   14bit*/
typedef struct BMP_FILE_HEADER {
    uint16_t bType;                         //File identifier
    uint32_t bSize;                         //The size of the file
    uint16_t bReserved1;                    //Reserved value, must be set to 0
    uint16_t bReserved2;                    //Reserved value, must be set to 0
    uint32_t bOffset;                       //The offset from the beginning of the file header to the beginning of the image data bit
} __attribute__((packed)) BMPFILEHEADER;  // 14bit


/*Bitmap information header  40bit*/
typedef struct BMP_INFO {
    uint32_t biInfoSize;       //The size of the header
    uint32_t biWidth;          //The width of the image
    uint32_t biHeight;         //The height of the image
    uint16_t biPlanes;         //The number of planes in the image
    uint16_t biBitCount;       //The number of bits per pixel
    uint32_t biCompression;    //Compression type
    uint32_t bimpImageSize;    //The size of the image, in bytes
    uint32_t biXPelsPerMeter;  //Horizontal resolution
    uint32_t biYPelsPerMeter;  //Vertical resolution
    uint32_t biClrUsed;        //The number of colors used
    uint32_t biClrImportant;   //The number of important colors
} __attribute__((packed)) BMPINFOHEADER;

class ePaperPort {
  private:
    spi_device_handle_t spi;
    uint32_t            i2c_data_pdMS_TICKS = 0;
    uint32_t            i2c_done_pdMS_TICKS = 0;
    const char         *TAG                 = "Display";
    int                 mosi_;
    int                 scl_;
    int                 dc_;
    int                 cs_;
    int                 rst_;
    int                 busy_;
    int                 width_;
    int                 height_;
    uint8_t            *DispBuffer = NULL;
    uint8_t            *RotationBuffer = NULL;
    uint8_t            *BmpSrcBuffer = NULL;
    int                 DisplayLen;
    int                 src_width;
    int                 src_height;
    uint8_t Rotation = 0;                          //0:0 1:90 2:180 3:270
    uint8_t mirrx = 0;                             
    uint8_t mirry = 0;

    void    Set_ResetIOLevel(uint8_t level);
    void    Set_CSIOLevel(uint8_t level);
    void    Set_DCIOLevel(uint8_t level);
    uint8_t Get_BusyIOLevel();
    void    EPD_Reset(void);
    void    EPD_LoopBusy(void);
    void    SPI_Write(uint8_t data);
    void    EPD_SendCommand(uint8_t Reg);
    void    EPD_SendData(uint8_t Data);
    void    EPD_Sendbuffera(uint8_t *Data, int len);
    void    EPD_TurnOnDisplay(void);
    uint8_t EPD_ColorToePaperColor(uint8_t b,uint8_t g,uint8_t r);
    uint8_t* EPD_ParseBMPImage(const char *path);
    uint8_t EPD_GetPixel4(const uint8_t* buf, int width, int x, int y);
    void    EPD_SetPixel4(uint8_t* buf, int width, int x, int y, uint8_t px);
    void EPD_Rotate180_Fast(const uint8_t* src, uint8_t* dst, int width, int height);
    void EPD_Rotate90CCW_Fast(const uint8_t* src, uint8_t* dst, int width, int height);
    void EPD_Rotate90CW_Fast(const uint8_t* src, uint8_t* dst, int width, int height);
    void EPD_PixelRotate();

  public:
    ePaperPort(int mosi, int scl, int dc, int cs, int rst, int busy, int width, int height, spi_host_device_t spihost = SPI3_HOST);
    ~ePaperPort();

    void EPD_Init();
    void EPD_DispClear(uint8_t color);
    void EPD_Display();
    void Set_Rotation(uint8_t rot); // 0:no 1:90 2:180 3:270
    void Set_Mirror(uint8_t mirr_x,uint8_t mirr_y);
    uint8_t* EPD_GetIMGBuffer();
    void EPD_SetPixel(uint16_t x, uint16_t y, uint16_t color);
    void EPD_SDcardBmpShakingColor(const char *path,uint16_t x_start, uint16_t y_start);
	void EPD_DrawStringCN(uint16_t Xstart, uint16_t Ystart, const char * pString, cFONT* font,uint16_t Color_Foreground, uint16_t Color_Background);
};