#pragma once

#include <esp_http_client.h>
#include "sdcard_bsp.h"
#include "jpegdecode_app.h"
#include "ArduinoJson.h"


/*HTTP POST callback, used to collect the JSON returned by Ark*/
typedef struct {
    char *buffer;
    int buffer_len;
} http_response_t;

typedef struct 
{
    int time;
    char url[100];
    char model[100];
    char key[100];
}BaseAIModelConfig_t;

class BaseAIModel : public JpegDecodeDither
{
private:
    CustomSDPort *SDPort_;
    const char *TAG = "AIModel";
    JsonDocument doc;
    char *ark_request_body = NULL;      // Chat message buffer
    const char *url = NULL;             // Volcano mode URL
    const char *apk = NULL;             // apk
    const char *model = NULL;           // Volcano model
    char *url_copy = NULL;              // Return the URL path of the image
    char sdcard_path[100] = {""};       // Return the final generated SD card path
    int path_value = 0;                 // SD card identifier symbol
    bool is_success = false;            // Flag indicating whether the image was successfully generated
    uint8_t *jpg_buffer = NULL;         // Store the JPG image data
    uint8_t *jpg_dec_buffer = NULL;     // Store the image after decoding JPG, no need to allocate memory, automatically allocated
    uint8_t *floyd_buffer = NULL;       // Store the data after applying the RGB888 jitter algorithm
    int width_;
    int height_;
    BaseAIModelConfig_t* AIModelConfig;
    http_response_t AIresponse = {0};

    static int BaseAIModel_HttpCallbackFun(esp_http_client_event_t *evt);
    const char* BaseAIModel_GetImgURL();                                            // Obtain the URL of the generated image
    uint8_t* BaseAIModel_DownloadImgToPsram(const char *strurl, int *out_len);      // Download the JPG image from the URL and save it to the PSRAM
    uint8_t BaseAIModel_PsramToSdcard(char *strPath,uint8_t *buffer,int len);       // Copy the data from the PSRAM to the SD card

public:
    BaseAIModel(CustomSDPort *SDPort,const int width,const int height);
    BaseAIModel(CustomSDPort *SDPort);
    ~BaseAIModel();
    BaseAIModelConfig_t* BaseAIModel_SdcardReadAIModelConfig();
    void BaseAIModel_AIModelInit(const char *ai_model, const char *ai_url, const char *ark_api_key);
    void BaseAIModel_SetChat(const char *str);                                      // Generate chat
    char *BaseAIModel_GetImgName();                                                 // Obtain the path of the last generated BMP file on the SDcard
};


