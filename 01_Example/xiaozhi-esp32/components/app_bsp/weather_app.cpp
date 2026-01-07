#include <stdio.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include "ArduinoJson.h"
#include "weather_app.h"

struct SpiRamAllocator : ArduinoJson::Allocator {
    void *allocate(size_t size) override {
        return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    }

    void deallocate(void *pointer) override {
        heap_caps_free(pointer);
    }

    void *reallocate(void *ptr, size_t new_size) override {
        return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
    }
};

SpiRamAllocator allocator;
JsonDocument    doc(&allocator);

WeatherPort::WeatherPort() {
    WeatherData = (WeatherData_t *) heap_caps_malloc(sizeof(WeatherData_t), MALLOC_CAP_SPIRAM);
    assert(WeatherData);
}

WeatherPort::~WeatherPort() {

}

void WeatherPort::WeatherPort_log(void) {
    ESP_LOGI(TAG, "Timer:%s", WeatherData->calendar);

    ESP_LOGI(TAG, "Today's Date:%s", WeatherData->td_weather);
    ESP_LOGI(TAG, "Today's Temperature:%s", WeatherData->td_Temp);
    ESP_LOGI(TAG, "Today's Wind Direction:%s", WeatherData->td_fx);
    ESP_LOGI(TAG, "Today's Weekday:%s", WeatherData->td_week);
    ESP_LOGI(TAG, "Today's Humidity:%s", WeatherData->td_RH);
    ESP_LOGI(TAG, "Today's Type:%s", WeatherData->td_type);

    ESP_LOGI(TAG, "Tomorrow's Date:%s", WeatherData->tmr_weather);
    ESP_LOGI(TAG, "Tomorrow's Temperature:%s", WeatherData->tmr_Temp);
    ESP_LOGI(TAG, "Tomorrow's Wind Direction:%s", WeatherData->tmr_fx);
    ESP_LOGI(TAG, "Tomorrow's Weekday:%s", WeatherData->tmr_week);
    ESP_LOGI(TAG, "Tomorrow's Humidity:%s", WeatherData->tmr_RH);
    ESP_LOGI(TAG, "Tomorrow's Type:%s", WeatherData->tmr_type);

    ESP_LOGI(TAG, "Day After Tomorrow's Date:%s", WeatherData->tdat_weather);
    ESP_LOGI(TAG, "Day After Tomorrow's Temperature:%s", WeatherData->tdat_Temp);
    ESP_LOGI(TAG, "Day After Tomorrow's Wind Direction:%s", WeatherData->tdat_fx);
    ESP_LOGI(TAG, "Day After Tomorrow's Weekday:%s", WeatherData->tdat_week);
    ESP_LOGI(TAG, "Day After Tomorrow's Humidity:%s", WeatherData->tdat_RH);
    ESP_LOGI(TAG, "Day After Tomorrow's Type:%s", WeatherData->tdat_type);

    ESP_LOGI(TAG, "Three Days Later Date:%s", WeatherData->stdat_weather);
    ESP_LOGI(TAG, "Three Days Later Temperature:%s", WeatherData->stdat_Temp);
    ESP_LOGI(TAG, "Three Days Later Wind Direction:%s", WeatherData->stdat_fx);
    ESP_LOGI(TAG, "Three Days Later Weekday:%s", WeatherData->stdat_week);
    ESP_LOGI(TAG, "Three Days Later Humidity:%s", WeatherData->stdat_RH);
    ESP_LOGI(TAG, "Three Days Later Type:%s", WeatherData->stdat_type);
}

WeatherData_t* WeatherPort::WeatherPort_DecodingSring(const char *jsonstr) {
    DeserializationError error = deserializeJson(doc, jsonstr);
    heap_caps_free((void *) jsonstr);
    jsonstr = NULL;
    if (error) {
        ESP_LOGE(TAG, "Analysis failed");
        return NULL;
    }
    int         wendu_high = 0;
    int         wendu_low  = 0;
    int         month      = 0;
    int         day        = 0;
    int         Numshidu   = 0;
    const char *str        = doc["time"];

    int s_year;
  	int s_month;
  	int s_day;
  	int s_hour;
  	int s_minute;
  	int s_second;
    sscanf(str, "%[^ ]", WeatherData->calendar); /* Time retrieval - accurate to the day */
    
    sscanf(str, "%d-%d-%d %d:%d:%d", &s_year, &s_month,&s_day, &s_hour,&s_minute, &s_second);
    snprintf(WeatherData->td_weather, 19, "%02d-%02d", s_month, s_day);

    str = doc["data"]["forecast"][0]["high"];
    sscanf(str, "%*[^0-9]%d", &wendu_high);
    str = doc["data"]["forecast"][0]["low"];
    sscanf(str, "%*[^0-9]%d", &wendu_low);
    snprintf(WeatherData->td_Temp, 30, "%02d-%02d℃", wendu_low, wendu_high);

    str = doc["data"]["forecast"][0]["fx"];
    strcpy(WeatherData->td_fx, str);

    str = doc["data"]["forecast"][0]["week"];
    strcpy(WeatherData->td_week, str);

    str = doc["data"]["shidu"];
    sscanf(str, "%d", &Numshidu);
    strcpy(WeatherData->td_RH, str);

    str = doc["data"]["forecast"][0]["type"];
    strcpy(WeatherData->td_type, str);

    WeatherData->td_aqi = doc["data"]["forecast"][0]["aqi"];
    /*Tomorrow's Weather*/
    str = doc["data"]["forecast"][1]["ymd"];
    sscanf(str, "%*[^-]-%d-%d", &month, &day);
    snprintf(WeatherData->tmr_weather, 19, "%02d-%02d", month, day);

    str = doc["data"]["forecast"][1]["high"];
    sscanf(str, "%*[^0-9]%d", &wendu_high);
    str = doc["data"]["forecast"][1]["low"];
    sscanf(str, "%*[^0-9]%d", &wendu_low);
    snprintf(WeatherData->tmr_Temp, 30, "%02d-%02d℃", wendu_low, wendu_high);

    str = doc["data"]["forecast"][1]["fx"];
    strcpy(WeatherData->tmr_fx, str);

    str = doc["data"]["forecast"][1]["week"];
    strcpy(WeatherData->tmr_week, str);

    snprintf(WeatherData->tmr_RH, 14, "%d%%", Numshidu + 1);

    str = doc["data"]["forecast"][1]["type"];
    strcpy(WeatherData->tmr_type, str);

    WeatherData->tmr_aqi = doc["data"]["forecast"][1]["aqi"];
    /*Weather Forecast for the Day After Tomorrow*/
    str = doc["data"]["forecast"][2]["ymd"];
    sscanf(str, "%*[^-]-%d-%d", &month, &day);
    snprintf(WeatherData->tdat_weather, 19, "%02d-%02d", month, day);

    str = doc["data"]["forecast"][2]["high"];
    sscanf(str, "%*[^0-9]%d", &wendu_high);
    str = doc["data"]["forecast"][2]["low"];
    sscanf(str, "%*[^0-9]%d", &wendu_low);
    snprintf(WeatherData->tdat_Temp, 30, "%02d-%02d℃", wendu_low, wendu_high);

    str = doc["data"]["forecast"][2]["fx"];
    strcpy(WeatherData->tdat_fx, str);

    str = doc["data"]["forecast"][2]["week"];
    strcpy(WeatherData->tdat_week, str);

    snprintf(WeatherData->tdat_RH, 14, "%d%%", Numshidu - 1);

    str = doc["data"]["forecast"][2]["type"];
    strcpy(WeatherData->tdat_type, str);

    WeatherData->tdat_aqi = doc["data"]["forecast"][2]["aqi"];

    /*The Weather the Day After Tomorrow*/
    str = doc["data"]["forecast"][3]["ymd"];
    sscanf(str, "%*[^-]-%d-%d", &month, &day);
    snprintf(WeatherData->stdat_weather, 19, "%02d-%02d", month, day);

    str = doc["data"]["forecast"][3]["high"];
    sscanf(str, "%*[^0-9]%d", &wendu_high);
    str = doc["data"]["forecast"][3]["low"];
    sscanf(str, "%*[^0-9]%d", &wendu_low);
    snprintf(WeatherData->stdat_Temp, 30, "%02d-%02d℃", wendu_low, wendu_high);

    str = doc["data"]["forecast"][3]["fx"];
    strcpy(WeatherData->stdat_fx, str);

    str = doc["data"]["forecast"][3]["week"];
    strcpy(WeatherData->stdat_week, str);

    snprintf(WeatherData->stdat_RH, 14, "%d%%", Numshidu - 2);

    str = doc["data"]["forecast"][3]["type"];
    strcpy(WeatherData->stdat_type, str);

    WeatherData->stdat_aqi = doc["data"]["forecast"][3]["aqi"];


    WeatherPort_log();

    return WeatherData;
}

char* WeatherPort::WeatherPort_GetSdCardImageName(const char *instr) {
    if (!strcmp("大雨", instr)) {
        strcpy(DirectoryImgName, "/sdcard/01_sys_init_img/01_dayu.bmp");
    } else if (!strcmp("多云", instr)) {
        strcpy(DirectoryImgName, "/sdcard/01_sys_init_img/02_duoyun.bmp");
    } else if (!strcmp("雷雨", instr)) {
        strcpy(DirectoryImgName, "/sdcard/01_sys_init_img/03_leiyu.bmp");
    } else if (!strcmp("晴", instr)) {
        strcpy(DirectoryImgName, "/sdcard/01_sys_init_img/04_qin.bmp");
    } else if (!strcmp("小雨", instr)) {
        strcpy(DirectoryImgName, "/sdcard/01_sys_init_img/05_xiaoyu.bmp");
    } else if (!strcmp("下雪", instr)) {
        strcpy(DirectoryImgName, "/sdcard/01_sys_init_img/06_xiaxue.bmp");
    } else if (!strcmp("中雨", instr)) {
        strcpy(DirectoryImgName, "/sdcard/01_sys_init_img/07_zhongyu.bmp");
    } else {
        strcpy(DirectoryImgName, "/sdcard/01_sys_init_img/08_yin.bmp");
    }
    return DirectoryImgName;
}

WeatherAqi_t WeatherPort::WeatherPort_GetWeatherAQI(int aqi) {
    if (aqi <= 50) {
        strcpy(WeatherAqi.str, "优");
        WeatherAqi.color = 0x06;
    } else if ((aqi > 50) && (aqi <= 100)) {
        strcpy(WeatherAqi.str, "良");
        WeatherAqi.color = 0x02;
    } else if ((aqi > 100) && (aqi <= 150)) {
        strcpy(WeatherAqi.str, "轻度污染");
        WeatherAqi.color = 0x03;
    } else {
        strcpy(WeatherAqi.str, "严重污染");
        WeatherAqi.color = 0x03;
    }
    return WeatherAqi;
}

uint16_t WeatherPort::WeatherPort_ReassignCoordinates(uint16_t x, const char *str) {
    uint16_t x_or;
    uint16_t len = strlen(str) / 3;
    if (len == 1) {
        x_or = 32 + x;
        return x_or;
    } else if (len == 2) {
        x_or = 20 + x;
        return x_or;
    } else if (len == 3) {
        x_or = 11 + x;
        return x_or;
    }
    return x;
}