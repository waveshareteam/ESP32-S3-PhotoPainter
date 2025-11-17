#include <stdio.h>
#include <string.h>
#include "json_data.h"
#include "ArduinoJson-v7.4.1.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "sdcard_bsp.h"

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


static void log_json_data_t(json_data_t *data) {
    const char *TAG = "json_data";
    ESP_LOGI(TAG, "rtc:%d/%02d/%02d %02d:%02d:%02d", data->rtc_data.year,
             data->rtc_data.month, data->rtc_data.day, data->rtc_data.hour,
             data->rtc_data.minute, data->rtc_data.second);
    ESP_LOGI(TAG, "Timer:%s", data->calendar);

    ESP_LOGI(TAG, "Today's Date:%s", data->td_weather);
    ESP_LOGI(TAG, "Today's Temperature:%s", data->td_Temp);
    ESP_LOGI(TAG, "Today's Wind Direction:%s", data->td_fx);
    ESP_LOGI(TAG, "Today's Weekday:%s", data->td_week);
    ESP_LOGI(TAG, "Today's Humidity:%s", data->td_RH);
    ESP_LOGI(TAG, "Today's Type:%s", data->td_type);

    ESP_LOGI(TAG, "Tomorrow's Date:%s", data->tmr_weather);
    ESP_LOGI(TAG, "Tomorrow's Temperature:%s", data->tmr_Temp);
    ESP_LOGI(TAG, "Tomorrow's Wind Direction:%s", data->tmr_fx);
    ESP_LOGI(TAG, "Tomorrow's Weekday:%s", data->tmr_week);
    ESP_LOGI(TAG, "Tomorrow's Humidity:%s", data->tmr_RH);
    ESP_LOGI(TAG, "Tomorrow's Type:%s", data->tmr_type);

    ESP_LOGI(TAG, "Day After Tomorrow's Date:%s", data->tdat_weather);
    ESP_LOGI(TAG, "Day After Tomorrow's Temperature:%s", data->tdat_Temp);
    ESP_LOGI(TAG, "Day After Tomorrow's Wind Direction:%s", data->tdat_fx);
    ESP_LOGI(TAG, "Day After Tomorrow's Weekday:%s", data->tdat_week);
    ESP_LOGI(TAG, "Day After Tomorrow's Humidity:%s", data->tdat_RH);
    ESP_LOGI(TAG, "Day After Tomorrow's Type:%s", data->tdat_type);

    ESP_LOGI(TAG, "Three Days Later Date:%s", data->stdat_weather);
    ESP_LOGI(TAG, "Three Days Later Temperature:%s", data->stdat_Temp);
    ESP_LOGI(TAG, "Three Days Later Wind Direction:%s", data->stdat_fx);
    ESP_LOGI(TAG, "Three Days Later Weekday:%s", data->stdat_week);
    ESP_LOGI(TAG, "Three Days Later Humidity:%s", data->stdat_RH);
    ESP_LOGI(TAG, "Three Days Later Type:%s", data->stdat_type);

}

SpiRamAllocator allocator;
JsonDocument    doc(&allocator);

json_data_t *json_read_data(const char *jsonStr) {
    json_data_t *str_data = (json_data_t *) heap_caps_malloc(sizeof(json_data_t), MALLOC_CAP_SPIRAM);
    if (str_data == NULL)
        return NULL;
    DeserializationError error = deserializeJson(doc, jsonStr);
    heap_caps_free((void *) jsonStr);
    jsonStr = NULL;
    if (error) {
        ESP_LOGE("json", "Analysis failed");
        return NULL;
    }
    int         wendu_high = 0;
    int         wendu_low  = 0;
    int         month      = 0;
    int         day        = 0;
    int         Numshidu   = 0;
    const char *str        = doc["time"];
    sscanf(str, "%d-%d-%d %d:%d:%d", &str_data->rtc_data.year, &str_data->rtc_data.month,
           &str_data->rtc_data.day, &str_data->rtc_data.hour,
           &str_data->rtc_data.minute, &str_data->rtc_data.second);
    sscanf(str, "%[^ ]", str_data->calendar); /* Time retrieval - accurate to the day */

    snprintf(str_data->td_weather, 19, "%02d-%02d", str_data->rtc_data.month, str_data->rtc_data.day);

    str = doc["data"]["forecast"][0]["high"];
    sscanf(str, "%*[^0-9]%d", &wendu_high);
    str = doc["data"]["forecast"][0]["low"];
    sscanf(str, "%*[^0-9]%d", &wendu_low);
    snprintf(str_data->td_Temp, 30, "%02d-%02d℃", wendu_low, wendu_high);

    str = doc["data"]["forecast"][0]["fx"];
    strcpy(str_data->td_fx, str);

    str = doc["data"]["forecast"][0]["week"];
    strcpy(str_data->td_week, str);

    str = doc["data"]["shidu"];
    sscanf(str, "%d", &Numshidu);
    strcpy(str_data->td_RH, str);

    str = doc["data"]["forecast"][0]["type"];
    strcpy(str_data->td_type, str);

    str_data->td_aqi = doc["data"]["forecast"][0]["aqi"];
    /*Tomorrow's Weather*/
    str = doc["data"]["forecast"][1]["ymd"];
    sscanf(str, "%*[^-]-%d-%d", &month, &day);
    snprintf(str_data->tmr_weather, 19, "%02d-%02d", month, day);

    str = doc["data"]["forecast"][1]["high"];
    sscanf(str, "%*[^0-9]%d", &wendu_high);
    str = doc["data"]["forecast"][1]["low"];
    sscanf(str, "%*[^0-9]%d", &wendu_low);
    snprintf(str_data->tmr_Temp, 30, "%02d-%02d℃", wendu_low, wendu_high);

    str = doc["data"]["forecast"][1]["fx"];
    strcpy(str_data->tmr_fx, str);

    str = doc["data"]["forecast"][1]["week"];
    strcpy(str_data->tmr_week, str);

    snprintf(str_data->tmr_RH, 14, "%d%%", Numshidu + 1);

    str = doc["data"]["forecast"][1]["type"];
    strcpy(str_data->tmr_type, str);

    str_data->tmr_aqi = doc["data"]["forecast"][1]["aqi"];
    /*Weather Forecast for the Day After Tomorrow*/
    str = doc["data"]["forecast"][2]["ymd"];
    sscanf(str, "%*[^-]-%d-%d", &month, &day);
    snprintf(str_data->tdat_weather, 19, "%02d-%02d", month, day);

    str = doc["data"]["forecast"][2]["high"];
    sscanf(str, "%*[^0-9]%d", &wendu_high);
    str = doc["data"]["forecast"][2]["low"];
    sscanf(str, "%*[^0-9]%d", &wendu_low);
    snprintf(str_data->tdat_Temp, 30, "%02d-%02d℃", wendu_low, wendu_high);

    str = doc["data"]["forecast"][2]["fx"];
    strcpy(str_data->tdat_fx, str);

    str = doc["data"]["forecast"][2]["week"];
    strcpy(str_data->tdat_week, str);

    snprintf(str_data->tdat_RH, 14, "%d%%", Numshidu - 1);

    str = doc["data"]["forecast"][2]["type"];
    strcpy(str_data->tdat_type, str);

    str_data->tdat_aqi = doc["data"]["forecast"][2]["aqi"];

    /*The Weather the Day After Tomorrow*/
    str = doc["data"]["forecast"][3]["ymd"];
    sscanf(str, "%*[^-]-%d-%d", &month, &day);
    snprintf(str_data->stdat_weather, 19, "%02d-%02d", month, day);

    str = doc["data"]["forecast"][3]["high"];
    sscanf(str, "%*[^0-9]%d", &wendu_high);
    str = doc["data"]["forecast"][3]["low"];
    sscanf(str, "%*[^0-9]%d", &wendu_low);
    snprintf(str_data->stdat_Temp, 30, "%02d-%02d℃", wendu_low, wendu_high);

    str = doc["data"]["forecast"][3]["fx"];
    strcpy(str_data->stdat_fx, str);

    str = doc["data"]["forecast"][3]["week"];
    strcpy(str_data->stdat_week, str);

    snprintf(str_data->stdat_RH, 14, "%d%%", Numshidu - 2);

    str = doc["data"]["forecast"][3]["type"];
    strcpy(str_data->stdat_type, str);

    str_data->stdat_aqi = doc["data"]["forecast"][3]["aqi"];

    /*end*/

    log_json_data_t(str_data);

    return str_data;
}

char *getSdCardImageDirectory(const char *instr) {
    static char str[50] = {" "};
    if (!strcmp("大雨", instr)) {
        strcpy(str, "/sdcard/01_sys_init_img/01_dayu.bmp");
    } else if (!strcmp("多云", instr)) {
        strcpy(str, "/sdcard/01_sys_init_img/02_duoyun.bmp");
    } else if (!strcmp("雷雨", instr)) {
        strcpy(str, "/sdcard/01_sys_init_img/03_leiyu.bmp");
    } else if (!strcmp("晴", instr)) {
        strcpy(str, "/sdcard/01_sys_init_img/04_qin.bmp");
    } else if (!strcmp("小雨", instr)) {
        strcpy(str, "/sdcard/01_sys_init_img/05_xiaoyu.bmp");
    } else if (!strcmp("下雪", instr)) {
        strcpy(str, "/sdcard/01_sys_init_img/06_xiaxue.bmp");
    } else if (!strcmp("中雨", instr)) {
        strcpy(str, "/sdcard/01_sys_init_img/07_zhongyu.bmp");
    } else if (!strcmp("阴", instr)) {
        strcpy(str, "/sdcard/01_sys_init_img/08_yin.bmp");
    } else {
        strcpy(str, "/sdcard/01_sys_init_img/qin.bmp");
    }
    return str;
}

json_aqi_t getWeatherAQI(int aqi) {
    json_aqi_t aqi_data;
    if (aqi <= 50) {
        strcpy(aqi_data.str, "优");
        aqi_data.color = 0x06;
    } else if ((aqi > 50) && (aqi <= 100)) {
        strcpy(aqi_data.str, "良");
        aqi_data.color = 0x02;
    } else if ((aqi > 100) && (aqi <= 150)) {
        strcpy(aqi_data.str, "轻度污染");
        aqi_data.color = 0x03;
    } else {
        strcpy(aqi_data.str, "严重污染");
        aqi_data.color = 0x03;
    }
    return aqi_data;
}


uint16_t reassignCoordinates(uint16_t x, const char *str) {
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

ai_model_t *json_sdcard_txt_aimodel(void) {
    ai_model_t *data = (ai_model_t *)malloc(sizeof(ai_model_t));
    uint8_t *sdcard_buffer = (uint8_t *)malloc(1024);
    assert(sdcard_buffer);
    assert(data);
    if(sdcard_read_file("/sdcard/06_user_foundation_img/config.txt", sdcard_buffer, NULL) != ESP_OK) {
        free(sdcard_buffer);
        sdcard_buffer = NULL;
        free(data);
        data = NULL;
        return NULL;
    }
    DeserializationError error = deserializeJson(doc, sdcard_buffer);
    free(sdcard_buffer);
    sdcard_buffer = NULL;
    if (error) {
        ESP_LOGE("sdcardjson", "Parsing failed");
        return NULL;
    }
    data->time        = doc["timer"];
    if(data->time == 0) {
        ESP_LOGE("sdcardjson", "Timer parsing failed");
        return NULL;
    }
    const char *str   = doc["ai_model"];
    if(str == NULL) {
        ESP_LOGE("sdcardjson", "AI model parsing failed");
        return NULL;
    }
    strcpy(data->model,str);
    str   = doc["ai_url"];
    if(str == NULL) {
        ESP_LOGE("sdcardjson", "AI URL parsing failed");
        return NULL;
    }
    strcpy(data->url,str);
    str   = doc["ai_key"];
    if(str == NULL) {
        ESP_LOGE("sdcardjson", "AI key parsing failed");
        return NULL;
    }
    strcpy(data->key,str);
    return data;
}
