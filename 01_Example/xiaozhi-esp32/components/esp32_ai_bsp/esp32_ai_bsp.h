#ifndef ESP32_AI_BSP_H
#define ESP32_AI_BSP_H

#include "ArduinoJson-v7.4.1.h"
#include "esp_http_client.h"
#include "floyd_steinberg.h"

/* HTTP POST callback, used to collect the JSON returned by Ark*/
typedef struct {
    char *buffer;
    int buffer_len;
} http_response_t;

class esp32_ai_bsp : public floyd_steinberg
{
private:
    JsonDocument doc;
    char *ark_request_body = NULL;      // Chat message buffer
    const char *url = NULL;             // Volcano mode URL
    const char *apk = NULL;             //apk
    const char *model = NULL;           // Volcano model
    char *url_copy = NULL;              // Return the URL path of the image
    char sdcard_path[100] = {""};       // Return the final generated SD card path
    int path_value = 0;                 // SD card identifier symbol
    bool is_success = false;            // Flag indicating whether the image was successfully generated
    uint8_t *jpg_buffer = NULL;         // Store the JPG image data
    uint8_t *jpg_dec_buffer = NULL;     // Store the image after decoding JPG, no need to allocate memory, automatically allocated
    uint8_t *floyd_buffer = NULL;       // Store the data after applying the RGB888 jitter algorithm
    int _width;
    int _height;

    static int _http_event_handler(esp_http_client_event_t *evt);
    const char* ark_get_image_url();     // Obtain the URL of the generated image
    uint8_t* download_image_to_psram(const char *strurl, int *out_len); // Download the JPG image from the URL and save it to the PSRAM
    uint8_t image_to_sdcard(char *strPath,uint8_t *buffer,int len);     // Copy the data from the PSRAM to the SD card
public:
    esp32_ai_bsp(const char *ai_model,const char *ai_url,const char *ark_api_key,const int width,const int height);
    ~esp32_ai_bsp();
    void set_Chat(const char *str);       // Generate chat
    char *get_ImgName();                  // Obtain the path of the last generated BMP file on the SDcard
};


#endif
