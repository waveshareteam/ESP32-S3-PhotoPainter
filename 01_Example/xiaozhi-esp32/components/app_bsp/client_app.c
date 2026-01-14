/********************************************************
目前该文件的内容仅支持中国大陆的天气获取，定位可以支持到中国大陆以外，但是目前没有对中国大陆以外的的位置信息进行处理，
因此该文件中存在中文打印以及中文注释，请谅解
如果您有比较友好的中国大陆以外天气及WiFi定位，欢迎反馈
At present, the content of this document only supports weather acquisition in Chinese mainland. Positioning can be supported outside Chinese mainland, but the location information outside Chinese mainland has not been processed yet.
Therefore, there are Chinese prints and Chinese annotations in this document. We apologize for any inconvenience
If you have friendly weather and WiFi location information outside Chinese mainland, please feel free to give us your feedback
********************************************************/
#include "client_app.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_log.h"

#define MIN(x, y) ((x < y) ? (x) : (y))

static const char *TAG = "client_bsp";

char province[64] = {0};
char city[64]     = {0};

extern const char api_root_cert_pem_start[] asm("_binary_api_root_cert_pem_start");
extern const char api_root_cert_pem_end[] asm("_binary_api_root_cert_pem_end");

#define MAX_HTTP_OUTPUT_BUFFER (1024 * 6)
#define UserDateURL "http://t.weather.sojson.com/api/weather/city/101280601"
#define AMAP_IP_URL "http://restapi.amap.com/v3/ip?key=0113a13c88697dcea6a445584d535837"

/*
HTTP_EVENT_ERROR	请求出错
HTTP_EVENT_ON_CONNECTED	建立连接成功
HTTP_EVENT_HEADER_SENT	请求头已发送
HTTP_EVENT_ON_HEADER	收到响应头字段
HTTP_EVENT_ON_DATA	  收到响应体数据(可能多次)
HTTP_EVENT_ON_FINISH	响应体接收完毕
HTTP_EVENT_DISCONNECTED	连接断开，检查 TLS 错误等
HTTP_EVENT_REDIRECT	收到重定向，决定是否跳转
*/
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    static char *output_buffer; // Buffer to store response of http request from event handler
    static int   output_len;    // Stores number of bytes read
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        output_len = 0;
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        // Clean the buffer in case of a new request
        if (output_len == 0 && evt->user_data) {
            // we are just starting to copy the output data into the use
            memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
        }
        if (!esp_http_client_is_chunked_response(evt->client)) {
            // If user_data buffer is configured, copy the response into the buffer
            int copy_len = 0;
            if (evt->user_data) {
                // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                if (copy_len) {
                    memcpy(evt->user_data + output_len, evt->data, copy_len);
                }
            } else {
                int content_len = esp_http_client_get_content_length(evt->client);
                if (output_buffer == NULL) {
                    // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                    output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                    output_len    = 0;
                    if (output_buffer == NULL) {
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                copy_len = MIN(evt->data_len, (content_len - output_len));
                if (copy_len) {
                    memcpy(output_buffer + output_len, evt->data, copy_len);
                }
            }
            output_len += copy_len;
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }

    return ESP_OK;
}

const char *fetch_weather_data(void) {

    char                    *local_response_buffer = (char *) heap_caps_malloc(MAX_HTTP_OUTPUT_BUFFER + 1, MALLOC_CAP_SPIRAM);
    esp_http_client_config_t config =
        {
            .url                   = UserDateURL,
            .event_handler         = _http_event_handler,
            .user_data             = local_response_buffer,
            .disable_auto_redirect = true,
        };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    // GET
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    //ESP_LOG_BUFFER_HEX(TAG, local_response_buffer, strlen(local_response_buffer));
    //printf("str:%s\n",local_response_buffer);
    //heap_caps_free(local_response_buffer);
    //local_response_buffer = NULL;
    esp_http_client_cleanup(client);

    return local_response_buffer;
}

// 去除“省”“市”后缀
// 去除结尾的“省”“市”“区”“县”“盟”“自治州”“特别行政区”
void trim_suffix(char *str) {
    size_t len = strlen(str);
    // 先处理“自治区”“自治州”“特别行政区”
    if (len >= 12 && strcmp(str + len - 12, "特别行政区") == 0) {
        str[len - 12] = '\0';
    } else if (len >= 9 && strcmp(str + len - 9, "自治州") == 0) {
        str[len - 9] = '\0';
    } else if (len >= 9 && strcmp(str + len - 9, "自治区") == 0) {
        str[len - 9] = '\0';
    } else if (len >= 3) {
        // 常见后缀
        const char *suffixes[] = {"省", "市", "区", "县", "盟"};
        for (int i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); ++i) {
            size_t suf_len = strlen(suffixes[i]);
            if (len >= suf_len && strcmp(str + len - suf_len, suffixes[i]) == 0) {
                str[len - suf_len] = '\0';
                break;
            }
        }
    }
}

// automatic orientation
int fetch_adcode(char *adcode_buf, size_t buf_len) {
    char *local_response_buffer = (char *) heap_caps_malloc(MAX_HTTP_OUTPUT_BUFFER + 1, MALLOC_CAP_SPIRAM);
    if (!local_response_buffer)
        return 0;

    esp_http_client_config_t config = {
        .url                   = AMAP_IP_URL,
        .event_handler         = _http_event_handler,
        .user_data             = local_response_buffer,
        .disable_auto_redirect = true,
        // .cert_pem = api_root_cert_pem_start,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t                err    = esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        heap_caps_free(local_response_buffer);
        return 0;
    }

    cJSON *root = cJSON_Parse(local_response_buffer);
    heap_caps_free(local_response_buffer);
    if (!root)
        return 0;

    cJSON *province_item = cJSON_GetObjectItem(root, "province");
    cJSON *city_item     = cJSON_GetObjectItem(root, "city");
    int    ret           = 0;
    if (province_item && cJSON_IsString(province_item) && city_item && cJSON_IsString(city_item)) {
        strncpy(province, province_item->valuestring, sizeof(province) - 1);
        province[sizeof(province) - 1] = '\0';
        strncpy(city, city_item->valuestring, sizeof(city) - 1);
        city[sizeof(city) - 1] = '\0';

        // Remove the suffix
        trim_suffix(province);
        trim_suffix(city);

        FILE *fp = fopen("/sdcard/01_sys_init_img/city_code.txt", "r");
        if (fp) {
            char line[128];
            while (fgets(line, sizeof(line), fp)) {
                char file_prov[64], file_city[64], file_code[32];
                if (sscanf(line, "%15[^,],%15[^,],%15s", file_prov, file_city, file_code) == 3) {
                    // ESP_LOGI(TAG, "查找: txt省市=%s %s, 定位省市=%s %s", file_prov, file_city, province, city);
                    if (strcmp(file_prov, province) == 0 && strcmp(file_city, city) == 0) {
                        strncpy(adcode_buf, file_code, buf_len - 1);
                        adcode_buf[buf_len - 1] = '\0';
                        ret                     = 1;
                        break;
                    }
                }
            }
            fclose(fp);
        } else {
            ESP_LOGI("SDCARD", "The file has been opened successfully.");
            fclose(fp);
        }
    }
    // ESP_LOGI(TAG, "定位省市: %s %s", province, city);
    // ESP_LOGI(TAG, "查找编码: %s", adcode_buf);
    cJSON_Delete(root);
    return ret;
}

// Obtain weather data
const char *fetch_weather_data_by_adcode(const char *adcode) {
    char url[128];
    snprintf(url, sizeof(url), "http://t.weather.sojson.com/api/weather/city/%s", adcode);
    ESP_LOGI(TAG, "天气API地址: %s", url);

    char *local_response_buffer = (char *) heap_caps_malloc(MAX_HTTP_OUTPUT_BUFFER + 1, MALLOC_CAP_SPIRAM);
    if (!local_response_buffer)
        return NULL;

    esp_http_client_config_t config = {
        .url                   = url,
        .event_handler         = _http_event_handler,
        .user_data             = local_response_buffer,
        .disable_auto_redirect = true,
        // .cert_pem = NULL,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t                err    = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);

    return local_response_buffer;
}

// Automatically locate and obtain the main weather process
void auto_get_weather(void) {
    char adcode[16] = {0};
    if (fetch_adcode(adcode, sizeof(adcode))) {
        // ESP_LOGI(TAG, "当前定位 adcode: %s", adcode);
        const char *weather_json = fetch_weather_data_by_adcode(adcode);
        if (weather_json) {
            // ESP_LOGI(TAG, "天气数据: %s", weather_json);
            heap_caps_free((void *) weather_json);
        } else {
            ESP_LOGE(TAG, "天气数据获取失败");
        }
    } else {
        ESP_LOGE(TAG, "定位失败，无法获取 adcode");
    }
}

const char *auto_get_weather_json(void) {
    char adcode[16] = {0};
    // fetch_adcode(adcode, sizeof(adcode));
    // const char *weather_json = fetch_weather_data();
    // ESP_LOGI(TAG, "天气数据: %s", weather_json);
    // return weather_json;
    if (fetch_adcode(adcode, sizeof(adcode))) {
        const char *weather_json = fetch_weather_data_by_adcode(adcode);
        if (weather_json) {
            // ESP_LOGI(TAG, "天气数据: %s", weather_json);
            ESP_LOGI(TAG, "天气数据获取成功");
            return weather_json;
        } else {
            ESP_LOGE(TAG, "天气数据获取失败");
        }
    } else {
        ESP_LOGE(TAG, "定位失败，无法获取 adcode");
    }
    return NULL;
}
