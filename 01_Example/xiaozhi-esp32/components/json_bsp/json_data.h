#ifndef JSON_DATA_H
#define JSON_DATA_H

#include "i2c_equipment.h"

typedef struct
{
    RtcDateTime_t rtc_data;
    char calendar[33];    //2025-08-08
    
    char td_weather[20];  // Date
    char td_Temp[32];     // Temperature - Low to High Data
    char td_fx[12];       // Wind Direction
    char td_week[12];     // Day of the Week
    char td_RH[15];       // Humidity
    char td_type[12];     // Corresponding Picture
    int td_aqi;           // Air Quality
    
    char tmr_weather[20];  // Date
    char tmr_Temp[32];     // Temperature - Low to High Data
    char tmr_fx[12];       // Wind Direction
    char tmr_week[12];     // Day of the Week
    char tmr_RH[15];       // Humidity
    char tmr_type[12];     // Corresponding Picture
    int tmr_aqi;           // Air Quality
    
    char tdat_weather[20];  // Date
    char tdat_Temp[32];     // Temperature - Low to High Data
    char tdat_fx[12];       // Wind Direction
    char tdat_week[12];     // Day of the Week
    char tdat_RH[15];       // Humidity
    char tdat_type[12];     // Corresponding Picture
    int tdat_aqi;           // Air Quality
    
    char stdat_weather[20];  // Date
    char stdat_Temp[32];     // Temperature - Low to High Data
    char stdat_fx[12];       // Wind Direction
    char stdat_week[12];     // Day of the Week
    char stdat_RH[15];       // Humidity
    char stdat_type[12];     // Corresponding Picture
    int stdat_aqi;           // Air Quality
}json_data_t;

typedef struct
{
    char str[20];           // Store the corresponding Chinese text
    uint8_t color;          // Background color
}json_aqi_t;

typedef struct 
{
    int time;
    char url[100];
    char model[100];
    char key[100];
}ai_model_t;

json_data_t *json_read_data(const char *jsonStr);
char *getSdCardImageDirectory(const char *instr);
json_aqi_t getWeatherAQI(int aqi);
uint16_t reassignCoordinates(uint16_t x,const char *str);
ai_model_t *json_sdcard_txt_aimodel(void);

#endif



