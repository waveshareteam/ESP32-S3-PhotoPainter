#include <stdio.h>
#include <string.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include "ai_app.h"

/*Volcano Engine CA Certificate*/
extern const uint8_t ark_vol_pem_start[] asm("_binary_ark_vol_pem_start");
/*Download the CA certificate of Volcano Engine image*/
extern const uint8_t ark_volces_chain_pem_start[] asm("_binary_volces_chain_pem_start");

BaseAIModel::BaseAIModel(CustomSDPort *SDPort, const int width, const int height) : 
SDPort_(SDPort)
{
    width_           = width;
    height_          = height;
    ark_request_body = (char *) heap_caps_malloc(3 * 1024, MALLOC_CAP_SPIRAM);              // Store chat messages
    url_copy         = (char *) heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);                  // Store the URL of the obtained image
    jpg_buffer       = (uint8_t *) heap_caps_malloc(width * height * 3, MALLOC_CAP_SPIRAM); // Store the JPG data
    floyd_buffer     = (uint8_t *) heap_caps_malloc(width * height * 3, MALLOC_CAP_SPIRAM); // Store the data after applying the RGB888 jitter algorithm
    AIModelConfig    = (BaseAIModelConfig_t *) heap_caps_malloc(sizeof(BaseAIModelConfig_t),MALLOC_CAP_SPIRAM);
    assert(ark_request_body);
    assert(url_copy);
    assert(jpg_buffer);
    assert(floyd_buffer);
    assert(AIModelConfig);
}

BaseAIModel::BaseAIModel(CustomSDPort *SDPort):
SDPort_(SDPort) 
{
    AIModelConfig    = (BaseAIModelConfig_t *) heap_caps_malloc(sizeof(BaseAIModelConfig_t),MALLOC_CAP_SPIRAM);
    assert(AIModelConfig);
}

void BaseAIModel::BaseAIModel_AIModelInit(const char *ai_model, const char *ai_url, const char *ark_api_key) {
    model            = ai_model;
    url              = ai_url;
    apk              = ark_api_key;
}

BaseAIModel::~BaseAIModel() {
}

int BaseAIModel::BaseAIModel_HttpCallbackFun(esp_http_client_event_t *evt) {
    http_response_t *resp = (http_response_t *) evt->user_data;
    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        if (!esp_http_client_is_chunked_response(evt->client)) {
            int copy_len = evt->data_len;
            resp->buffer = (char *) realloc(resp->buffer, resp->buffer_len + copy_len + 1);
            memcpy(resp->buffer + resp->buffer_len, evt->data, copy_len);
            resp->buffer_len += copy_len;
            resp->buffer[resp->buffer_len] = '\0';
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

const char *BaseAIModel::BaseAIModel_GetImgURL() {
    http_response_t response;

    esp_http_client_config_t config = {};
    config.url                      = url;
    config.event_handler            = BaseAIModel_HttpCallbackFun;
    config.user_data                = &response;
    config.cert_pem                 = (const char *) ark_vol_pem_start;
    esp_http_client_handle_t client = esp_http_client_init(&config);

    char auth_header[128];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", apk);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_post_field(client, ark_request_body, strlen(ark_request_body));

    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Ark request failed: %s", esp_err_to_name(err));
        free(response.buffer);
        response.buffer = NULL;
        return NULL;
    }

    ESP_LOGE(TAG, "url:%s", url);
    ESP_LOGE(TAG, "apk:%s", apk);
    ESP_LOGE(TAG, "model:%s", model);
    ESP_LOGI(TAG, "Ark response: %s", response.buffer);

    JsonDocument         desDoc;
    DeserializationError error = deserializeJson(desDoc, response.buffer);
    free(response.buffer);
    response.buffer = NULL;

    if (error) {
        ESP_LOGE(TAG, "JSON parse failed");
        return NULL;
    }

    const char *url_str = desDoc["data"][0]["url"];
    if (url_str == NULL) {
        return NULL;
    }

    ESP_LOGI(TAG, "Image URL: %s", url_str);

    strcpy(url_copy, url_str);
    url_copy[strlen(url_copy)] = 0;
    return url_copy;
}

uint8_t *BaseAIModel::BaseAIModel_DownloadImgToPsram(const char *strurl, int *out_len) {
    http_response_t response = {0};

    if (strurl == NULL) {
        ESP_LOGE(TAG, "img url NUll");
        return NULL;
    }
    ESP_LOGE("IMG URL", "%s", strurl);
    esp_http_client_config_t config = {};
    config.url                      = strurl;
    config.event_handler            = BaseAIModel_HttpCallbackFun;
    config.user_data                = &response;
    config.cert_pem                 = (const char *) ark_volces_chain_pem_start; // Some URLs may be in https format.
    config.buffer_size              = 4096;                                      // The default size is 1024, which has been increased to 4 KB.
    config.buffer_size_tx           = 2048;                                      // Send buffering
    config.timeout_ms               = 10000;                                     // Set the timeout to 10 seconds.
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Image download failed: %s", esp_err_to_name(err));
        free(response.buffer);
        return NULL;
    }

    if (!jpg_buffer) {
        ESP_LOGE(TAG, "PSRAM malloc failed, size=%d", response.buffer_len);
        free(response.buffer);
        return NULL;
    }

    memcpy(jpg_buffer, response.buffer, response.buffer_len);
    if (out_len != NULL) {
        *out_len = response.buffer_len;
    }
    free(response.buffer);
    ESP_LOGI(TAG, "Image downloaded to PSRAM, size=%d bytes", response.buffer_len);
    return jpg_buffer;
}

uint8_t BaseAIModel::BaseAIModel_PsramToSdcard(char *strPath, uint8_t *buffer, int len) {
    if (SDPort_->SDPort_WriteFile(strPath, buffer, len) == ESP_OK) {
        return 1;
    }
    return 0;
}

void BaseAIModel::BaseAIModel_SetChat(const char *str) {
    doc["model"]           = model;
    doc["prompt"]          = str;
    doc["response_format"] = "url";
    doc["size"]            = "800x480";
    doc["guidance_scale"]  = 3;     //The larger the value, the more relevant the image generation will be.
    doc["watermark"]       = false; //No watermark required
    if (serializeJson(doc, ark_request_body, 3 * 1024) > 0) {
        is_success = true;
    } else {
        is_success = false;
    }
}

char *BaseAIModel::BaseAIModel_GetImgName() {
    if (!is_success) {
        ESP_LOGE(TAG, "set_chat fill");
        return NULL;
    }
    if (BaseAIModel_GetImgURL() == NULL) {
        ESP_LOGE(TAG, "read URL fill");
        return NULL;
    }
    int jpg_size = 0;
    if (BaseAIModel_DownloadImgToPsram(url_copy, &jpg_size) == NULL) {
        ESP_LOGE(TAG, "http get img data fill");
        return NULL;
    }
    int dec_jpg_size = 0;
    if (JpegPort_OnePicture(jpg_buffer, jpg_size, &jpg_dec_buffer, &dec_jpg_size) == 0) {
        ESP_LOGE(TAG, "jpg dec fill");
        if (jpg_dec_buffer != NULL) {
            JpegPort_BufferFree(jpg_dec_buffer);
        }
        return NULL;
    }
    JpegPort_DitherRgb888(jpg_dec_buffer, floyd_buffer, width_, height_);   //The RGB888 data has undergone the jittering algorithm.
    JpegPort_BufferFree(jpg_dec_buffer);                                    //Release memory
    snprintf(sdcard_path, 98, "/sdcard/05_user_ai_img/ai_%d.bmp", path_value);
    if (JpegPort_EncodingBmpToSdcard(sdcard_path, floyd_buffer, width_, height_) != 0) {
        ESP_LOGE(TAG, "rgb888 to sdcard is bmp fill");
        return NULL;
    }
    path_value++;
    return sdcard_path;
}

BaseAIModelConfig_t* BaseAIModel::BaseAIModel_SdcardReadAIModelConfig() {
    uint8_t *sdcard_buffer = (uint8_t *)malloc(1024);
    assert(sdcard_buffer);
    if(SDPort_->SDPort_ReadFile("/sdcard/06_user_foundation_img/config.txt", sdcard_buffer, NULL) != ESP_OK) {
        free(sdcard_buffer);
        sdcard_buffer = NULL;
        return NULL;
    }
    DeserializationError error = deserializeJson(doc, sdcard_buffer);
    free(sdcard_buffer);
    sdcard_buffer = NULL;
    if (error) {
        ESP_LOGE("sdcardjson", "Parsing failed");
        return NULL;
    }
    AIModelConfig->time        = doc["timer"];
    if(AIModelConfig->time == 0) {
        ESP_LOGE("sdcardjson", "Timer parsing failed");
        return NULL;
    }
    const char *str   = doc["ai_model"];
    if(str == NULL) {
        ESP_LOGE("sdcardjson", "AI model parsing failed");
        return NULL;
    }
    strcpy(AIModelConfig->model,str);
    str   = doc["ai_url"];
    if(str == NULL) {
        ESP_LOGE("sdcardjson", "AI URL parsing failed");
        return NULL;
    }
    strcpy(AIModelConfig->url,str);
    str   = doc["ai_key"];
    if(str == NULL) {
        ESP_LOGE("sdcardjson", "AI key parsing failed");
        return NULL;
    }
    strcpy(AIModelConfig->key,str);
    return AIModelConfig;
}