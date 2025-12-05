#pragma once

#include <freertos/FreeRTOS.h>

typedef struct {
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
}WeatherData_t;

typedef struct
{
    char str[20];           // Store the corresponding Chinese text
    uint8_t color;          // Background color
}WeatherAqi_t;

class WeatherPort
{
private:
    const char *TAG = "WeatherPort";
    WeatherData_t *WeatherData;
    char DirectoryImgName[50] = {""};
    WeatherAqi_t  WeatherAqi;
    void WeatherPort_log(void);
public:
    WeatherPort();
    ~WeatherPort();

    WeatherData_t* WeatherPort_DecodingSring(const char *str);
    char* WeatherPort_GetSdCardImageName(const char *instr);
    WeatherAqi_t WeatherPort_GetWeatherAQI(int aqi);
    uint16_t WeatherPort_ReassignCoordinates(uint16_t x, const char *str);
};



