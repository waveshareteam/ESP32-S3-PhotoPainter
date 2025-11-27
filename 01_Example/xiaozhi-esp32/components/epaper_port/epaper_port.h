#ifndef EPAPER_PORT_H
#define EPAPER_PORT_H


/**********************************
Color Index
**********************************/
#define EPD_7IN3E_BLACK   0x0 
#define EPD_7IN3E_WHITE   0x1 
#define EPD_7IN3E_YELLOW  0x2 
#define EPD_7IN3E_RED     0x3 
// #define EPD_7IN3E_ORANGE  0x4 
#define EPD_7IN3E_BLUE    0x5 
#define EPD_7IN3E_GREEN   0x6 


#define EXAMPLE_LCD_WIDTH  800
#define EXAMPLE_LCD_HEIGHT 480


/**********************************
Color Index
**********************************/
#define EPD_7IN3E_BLACK   0x0   /// 000
#define EPD_7IN3E_WHITE   0x1   /// 001
#define EPD_7IN3E_YELLOW  0x2   /// 010
#define EPD_7IN3E_RED     0x3   /// 011
#define EPD_7IN3E_BLUE    0x5   /// 101
#define EPD_7IN3E_GREEN   0x6   /// 110

#ifdef __cplusplus
extern "C" {
#endif


void epaper_port_init(void);
void epaper_port_clear(uint8_t *Image,uint8_t color);
void epaper_port_display(uint8_t *Image);


#ifdef __cplusplus
}
#endif


#endif // !EPAPER_PORT_H
