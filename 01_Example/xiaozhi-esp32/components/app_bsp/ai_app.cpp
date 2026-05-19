#include <stdio.h>
#include <string.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include "ai_app.h"

static constexpr int AI_RESPONSE_BUFFER_SIZE = 700 * 1024;
static constexpr int HTTP_RX_BUFFER_SIZE = 4096;
static constexpr int HTTP_TX_BUFFER_SIZE = 2048;
static constexpr int ARK_REQUEST_TIMEOUT_MS = 180000;
static constexpr int IMAGE_DOWNLOAD_TIMEOUT_MS = 60000;

/*Volcano Engine CA Certificate*/
extern const uint8_t ark_vol_pem_start[] asm("_binary_ark_vol_pem_start");
/*Download the CA certificate of Volcano Engine image*/
extern const uint8_t ark_volces_chain_pem_start[] asm("_binary_volces_chain_pem_start");

static void ResetHttpResponse(http_response_t *resp) {
    if (resp == NULL || resp->buffer == NULL || resp->buffer_size <= 0) {
        return;
    }

    resp->buffer_len = 0;
    resp->buffer[0] = '\0';
}

BaseAIModel::BaseAIModel(CustomSDPort *SDPort,ImgDecodeDither &dither, const int width, const int height) : 
SDPort_(SDPort),
dither_(dither)
{
    width_           = width;
    height_          = height;
    ark_request_body = (char *) heap_caps_malloc(3 * 1024, MALLOC_CAP_SPIRAM);              // Store chat messages
    url_copy         = (char *) heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);                  // Store the URL of the obtained image
    //jpg_buffer       = (uint8_t *) heap_caps_malloc(500 * 1024, MALLOC_CAP_SPIRAM);         // Store the JPG data
    //floyd_buffer     = (uint8_t *) heap_caps_malloc(width * height * 3, MALLOC_CAP_SPIRAM); // Store the data after applying the RGB888 jitter algorithm
    AIModelConfig    = (BaseAIModelConfig_t *) heap_caps_malloc(sizeof(BaseAIModelConfig_t),MALLOC_CAP_SPIRAM);
    AIresponse.buffer = (char *) heap_caps_malloc(AI_RESPONSE_BUFFER_SIZE,MALLOC_CAP_SPIRAM);
    AIresponse.buffer_size = AI_RESPONSE_BUFFER_SIZE;
    ResetHttpResponse(&AIresponse);
    assert(ark_request_body);
    assert(url_copy);
    //assert(jpg_buffer);
    //assert(floyd_buffer);
    assert(AIModelConfig);
    assert(AIresponse.buffer);
}

BaseAIModel::BaseAIModel(CustomSDPort *SDPort,ImgDecodeDither &dither):
SDPort_(SDPort),
dither_(dither)
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

esp_err_t BaseAIModel::BaseAIModel_HttpCallbackFun(esp_http_client_event_t *evt) {
    http_response_t *resp = (http_response_t *) evt->user_data;
    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        if (resp == NULL || resp->buffer == NULL || resp->buffer_size <= 0) {
            return ESP_FAIL;
        }
        if (evt->data_len > 0) {
            int remaining_len = resp->buffer_size - resp->buffer_len - 1;
            if (evt->data_len > remaining_len) {
                ESP_LOGE("AIModel", "HTTP response buffer overflow, received=%d, remaining=%d",
                         evt->data_len, remaining_len);
                return ESP_ERR_NO_MEM;
            }
            memcpy(resp->buffer + resp->buffer_len, evt->data, evt->data_len);
            resp->buffer_len += evt->data_len;
            resp->buffer[resp->buffer_len] = '\0';
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

const char *BaseAIModel::BaseAIModel_GetImgURL() {
    ResetHttpResponse(&AIresponse);
    esp_http_client_config_t config = {};
    config.url                      = url;
    config.event_handler            = BaseAIModel_HttpCallbackFun;
    config.user_data                = &AIresponse;
    config.cert_pem                 = (const char *) ark_vol_pem_start;
    config.buffer_size              = HTTP_RX_BUFFER_SIZE;
    config.buffer_size_tx           = HTTP_TX_BUFFER_SIZE;
    config.timeout_ms               = ARK_REQUEST_TIMEOUT_MS;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize Ark HTTP client");
        return NULL;
    }

    char auth_header[320];
    int auth_len = snprintf(auth_header, sizeof(auth_header), "Bearer %s", apk);
    if (auth_len < 0 || auth_len >= (int) sizeof(auth_header)) {
        ESP_LOGE(TAG, "Ark API key is too long");
        esp_http_client_cleanup(client);
        return NULL;
    }

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_post_field(client, ark_request_body, strlen(ark_request_body));

    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Ark request failed: %s", esp_err_to_name(err));
        ResetHttpResponse(&AIresponse);
        return NULL;
    }

    if (status_code < 200 || status_code >= 300) {
        ESP_LOGE(TAG, "Ark request failed, HTTP status=%d, response=%.*s",
                 status_code, AIresponse.buffer_len < 256 ? AIresponse.buffer_len : 256, AIresponse.buffer);
        ResetHttpResponse(&AIresponse);
        return NULL;
    }

    JsonDocument         desDoc;
    DeserializationError error = deserializeJson(desDoc, AIresponse.buffer);

    if (error) {
        ESP_LOGE(TAG, "JSON parse failed: %s, len=%d", error.c_str(), AIresponse.buffer_len);
        ResetHttpResponse(&AIresponse);
        return NULL;
    }

    const char *url_str = desDoc["data"][0]["url"];
    if (url_str == NULL) {
        ESP_LOGE(TAG, "Ark response does not contain data[0].url");
        ResetHttpResponse(&AIresponse);
        return NULL;
    }

    snprintf(url_copy, 1024, "%s", url_str);

    ResetHttpResponse(&AIresponse);
    return url_copy;
}

uint8_t *BaseAIModel::BaseAIModel_DownloadImgToPsram(const char *strurl, int *out_len) {
    if (strurl == NULL) {
        ESP_LOGE(TAG, "img url NULL");
        return NULL;
    }
    ResetHttpResponse(&AIresponse);
    ESP_LOGW("IMG URL", "%s", strurl);
    esp_http_client_config_t config = {};
    config.url                      = strurl;
    config.event_handler            = BaseAIModel_HttpCallbackFun;
    config.user_data                = &AIresponse;
    config.cert_pem                 = (const char *) ark_volces_chain_pem_start; // Some URLs may be in https format.
    config.buffer_size              = HTTP_RX_BUFFER_SIZE;                       // The default size is 1024, which has been increased to 4 KB.
    config.buffer_size_tx           = HTTP_TX_BUFFER_SIZE;                       // Send buffering
    config.timeout_ms               = IMAGE_DOWNLOAD_TIMEOUT_MS;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize image HTTP client");
        return NULL;
    }
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Image download failed: %s", esp_err_to_name(err));
        ResetHttpResponse(&AIresponse);
        return NULL;
    }

    if (status_code < 200 || status_code >= 300) {
        ESP_LOGE(TAG, "Image download failed, HTTP status=%d", status_code);
        ResetHttpResponse(&AIresponse);
        return NULL;
    }

    //if (!jpg_buffer) {
    //    ESP_LOGE(TAG, "PSRAM malloc failed, size=%d", AIresponse.buffer_len);
    //    return NULL;
    //}

    //memcpy(jpg_buffer, AIresponse.buffer, AIresponse.buffer_len);
    if (out_len != NULL) {
        *out_len = AIresponse.buffer_len;
    }
    ESP_LOGI(TAG, "Image downloaded to PSRAM, size=%d bytes", AIresponse.buffer_len);
    //return jpg_buffer;
    return (uint8_t *)AIresponse.buffer;
}

uint8_t BaseAIModel::BaseAIModel_PsramToSdcard(char *strPath, uint8_t *buffer, int len) {
    if (SDPort_->SDPort_WriteFile(strPath, buffer, len) == ESP_OK) {
        return 1;
    }
    return 0;
}

void BaseAIModel::BaseAIModel_SetChat(const char *str) {
    doc.clear();
    doc["model"]            = model;
    doc["prompt"]           = str;
    doc["response_format"]  = "url";
    doc["size"]             = "1280x768";
    doc["sequential_image_generation"]  = "disabled";     
    doc["stream"]           = false; 
    doc["watermark"]        = false; 
    if (serializeJson(doc, ark_request_body, 3 * 1024) > 0) {
        ESP_LOGW("AIModel","Body : %s",ark_request_body);
        is_success = true;
    } else {
        ESP_LOGE("AIModel","Body : NULL");
        is_success = false;
    }
}

char *BaseAIModel::BaseAIModel_GetImgName() {
    if (!is_success) {
        ESP_LOGE(TAG, "set_chat fail");
        return NULL;
    }
    if (BaseAIModel_GetImgURL() == NULL) {
        ESP_LOGE(TAG, "read URL fail");
        return NULL;
    }

    int jpg_size = 0;
    if (BaseAIModel_DownloadImgToPsram(url_copy, &jpg_size) == NULL) {
        ESP_LOGE(TAG, "http get img data fail");
        return NULL;
    }
#if 0
    int dec_jpg_size = 0;
    if (dither_.ImgDecode_OneJPGPicture((uint8_t *)AIresponse.buffer, jpg_size, &jpg_dec_buffer, &dec_jpg_size) != ESP_OK) {
        ESP_LOGE(TAG, "jpg dec fail");
        if (jpg_dec_buffer != NULL) {
            dither_.ImgDecode_JPGBufferFree(jpg_dec_buffer);
        }
        return NULL;
    }
    floyd_buffer     = (uint8_t *) heap_caps_malloc(width_ * height_ * 3, MALLOC_CAP_SPIRAM); // Store the data after applying the RGB888 jitter algorithm
    dither_.ImgDecode_DitherRgb888(jpg_dec_buffer, floyd_buffer, width_, height_);            // The RGB888 data has undergone the jittering algorithm.
    dither_.ImgDecode_JPGBufferFree(jpg_dec_buffer);                                          // Release memory
    strcpy(sdcard_path,"/sdcard/04_sys_ai_img/sys_ai.bmp");
#endif
    strcpy(sdcard_path,"/sdcard/04_sys_ai_img/sys_ai.jpg");
    if(ESP_OK == SDPort_->SDPort_WriteFile(sdcard_path, AIresponse.buffer, jpg_size)) {
        ESP_LOGI(TAG, "Image saved to SD card successfully");
    } else {
        ESP_LOGE(TAG, "Failed to save image to SD card");
        return NULL;
    }
//    if (dither_.ImgDecode_EncodingBmpToSdcard(sdcard_path, floyd_buffer, width_, height_) != ESP_OK) {
//        ESP_LOGE(TAG, "rgb888 to sdcard is bmp fail");
//        return NULL;
//    }
//    heap_caps_free(floyd_buffer);
//    floyd_buffer = NULL;
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
    snprintf(AIModelConfig->model, sizeof(AIModelConfig->model), "%s", str);
    str   = doc["ai_url"];
    if(str == NULL) {
        ESP_LOGE("sdcardjson", "AI URL parsing failed");
        return NULL;
    }
    snprintf(AIModelConfig->url, sizeof(AIModelConfig->url), "%s", str);
    str   = doc["ai_key"];
    if(str == NULL) {
        ESP_LOGE("sdcardjson", "AI key parsing failed");
        return NULL;
    }
    snprintf(AIModelConfig->key, sizeof(AIModelConfig->key), "%s", str);
    return AIModelConfig;
}
