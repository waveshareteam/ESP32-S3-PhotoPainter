#include <stdio.h>
#include <esp_check.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include "server_app.h"
#include "sdcard_bsp.h"
#include "mdns.h"
#include "button_bsp.h"

static const char *TAG = "server_bsp";

#define ServerPort_MIN(x, y) ((x < y) ? (x) : (y))
#define READ_LEN_MAX (10 * 1024) // Buffer area for receiving data
#define SEND_LEN_MAX (5 * 1024)  // Data for sending response

EventGroupHandle_t ServerPortGroups;
static CustomSDPort *SDPort_ = NULL;

/*Callback function*/
esp_err_t index_html_redirect_handler(httpd_req_t *req);
esp_err_t bootstrap_css_redirect_handler(httpd_req_t *req);
esp_err_t styles_css_redirect_handler(httpd_req_t *req);
esp_err_t script_js_redirect_handler(httpd_req_t *req);
esp_err_t bootstrap_js_redirect_handler(httpd_req_t *req);
esp_err_t img_data_redirect_handler(httpd_req_t *req);
esp_err_t unknown_uri_handler(httpd_req_t *req);

/*The callback function for handling GET requests*/
esp_err_t index_html_redirect_handler(httpd_req_t *req) {
    char       *buf;
    size_t      buf_len;
    const char *uri = req->uri;                                     // The desired URI
    buf_len         = httpd_req_get_hdr_value_len(req, "Host") + 1; // Obtain the length of the host address

    if (buf_len > 1) {
        buf = (char *) malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s URL ==> %s", buf,uri);
        }
        free(buf);
        buf = NULL;
    }

    /*Send response*/
    char  *resp_str      = (char *) heap_caps_malloc(SEND_LEN_MAX + 1, MALLOC_CAP_SPIRAM);
    size_t str_respLen   = 0;
    size_t str_len       = 0;
    //char   Name_str[125] = {""};
    //snprintf(Name_str, sizeof(Name_str), "/sdcard/03_sys_ap_html%.100s", uri);
    httpd_resp_set_type(req, "text/html");
    while (1) {
        str_len = SDPort_->SDPort_ReadOffset("/sdcard/03_sys_ap_html/index.html",resp_str,SEND_LEN_MAX,str_respLen);
        if (str_len) {
            httpd_resp_send_chunk(req, resp_str, str_len); //Send data
            str_respLen += str_len;
        } else {
            break;
        }
    }
    httpd_resp_send_chunk(req, NULL, 0);               // Send empty data to indicate completion of transmission
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) // After sending the response, check if there are any remaining request headers. The ESP32 will send again after this.
    {
        ESP_LOGI(TAG, "Request headers lost");
    }
    heap_caps_free(resp_str);
    resp_str = NULL;
    return ESP_OK;
}

esp_err_t bootstrap_css_redirect_handler(httpd_req_t *req) {
    char       *buf;
    size_t      buf_len;
    const char *uri = req->uri;                                     // The desired URI
    buf_len         = httpd_req_get_hdr_value_len(req, "Host") + 1; // Obtain the length of the host address

    if (buf_len > 1) {
        buf = (char *) malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s URL ==> %s", buf,uri);
        }
        free(buf);
        buf = NULL;
    }

    char  *resp_str      = (char *) heap_caps_malloc(SEND_LEN_MAX + 1, MALLOC_CAP_SPIRAM);
    size_t str_respLen   = 0;
    size_t str_len       = 0;
    httpd_resp_set_type(req, "text/css");
    while (1) {
        str_len = SDPort_->SDPort_ReadOffset("/sdcard/03_sys_ap_html/assets/bootstrap/css/bootstrap.min.css", resp_str, SEND_LEN_MAX, str_respLen);
        if (str_len) {
            httpd_resp_send_chunk(req, resp_str, str_len);
            str_respLen += str_len;
        } else {
            break;
        }
    }

    httpd_resp_send_chunk(req, NULL, 0);               // Send empty data to indicate completion of transmission
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) // After sending the response, check if there are any previous request headers left. The ESP32 will clear them after sending.
    {
        ESP_LOGI(TAG, "Request headers lost");
    }
    heap_caps_free(resp_str);
    resp_str = NULL;
    return ESP_OK;
}

esp_err_t styles_css_redirect_handler(httpd_req_t *req) {
    char       *buf;
    size_t      buf_len;
    const char *uri = req->uri;                                     // The desired URI
    buf_len         = httpd_req_get_hdr_value_len(req, "Host") + 1; // Obtain the length of the host address

    if (buf_len > 1) {
        buf = (char *) malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s URL ==> %s", buf,uri);
        }
        free(buf);
        buf = NULL;
    }

    char  *resp_str      = (char *) heap_caps_malloc(SEND_LEN_MAX + 1, MALLOC_CAP_SPIRAM);
    size_t str_respLen   = 0;
    size_t str_len       = 0;
    httpd_resp_set_type(req, "text/css");
    while (1) {
        str_len = SDPort_->SDPort_ReadOffset("/sdcard/03_sys_ap_html/assets/css/styles.min.css", resp_str, SEND_LEN_MAX, str_respLen);
        if (str_len) {
            httpd_resp_send_chunk(req, resp_str, str_len);
            str_respLen += str_len;
        } else {
            break;
        }
    }

    httpd_resp_send_chunk(req, NULL, 0);               // Send empty data to indicate completion of transmission
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) // After sending the response, check if there are any previous request headers left. The ESP32 will clear them after sending.
    {
        ESP_LOGI(TAG, "Request headers lost");
    }
    heap_caps_free(resp_str);
    resp_str = NULL;
    return ESP_OK;
}

esp_err_t script_js_redirect_handler(httpd_req_t *req) {
    char       *buf;
    size_t      buf_len;
    const char *uri = req->uri;                                     // The desired URI
    buf_len         = httpd_req_get_hdr_value_len(req, "Host") + 1; // Obtain the length of the host address

    if (buf_len > 1) {
        buf = (char *) malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s URL ==> %s", buf,uri);
        }
        free(buf);
        buf = NULL;
    }

    char  *resp_str      = (char *) heap_caps_malloc(SEND_LEN_MAX + 1, MALLOC_CAP_SPIRAM);
    size_t str_respLen   = 0;
    size_t str_len       = 0;
    httpd_resp_set_type(req, "application/javascript");
    while (1) {
        str_len = SDPort_->SDPort_ReadOffset("/sdcard/03_sys_ap_html/assets/js/script.min.js", resp_str, SEND_LEN_MAX, str_respLen);
        if (str_len) {
            httpd_resp_send_chunk(req, resp_str, str_len);
            str_respLen += str_len;
        } else {
            break;
        }
    }

    httpd_resp_send_chunk(req, NULL, 0);               // Send empty data to indicate completion of transmission
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) // After sending the response, check if there are any previous request headers left. The ESP32 will clear them after sending.
    {
        ESP_LOGI(TAG, "Request headers lost");
    }
    heap_caps_free(resp_str);
    resp_str = NULL;
    return ESP_OK;
}

esp_err_t bootstrap_js_redirect_handler(httpd_req_t *req) {
    char       *buf;
    size_t      buf_len;
    const char *uri = req->uri;                                     // The desired URI
    buf_len         = httpd_req_get_hdr_value_len(req, "Host") + 1; // Obtain the length of the host address

    if (buf_len > 1) {
        buf = (char *) malloc(buf_len);
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s URL ==> %s", buf,uri);
        }
        free(buf);
        buf = NULL;
    }

    char  *resp_str      = (char *) heap_caps_malloc(SEND_LEN_MAX + 1, MALLOC_CAP_SPIRAM);
    size_t str_respLen   = 0;
    size_t str_len       = 0;
    httpd_resp_set_type(req, "application/javascript");
    while (1) {
        str_len = SDPort_->SDPort_ReadOffset("/sdcard/03_sys_ap_html/assets/js/script.min.js", resp_str, SEND_LEN_MAX, str_respLen);
        if (str_len) {
            httpd_resp_send_chunk(req, resp_str, str_len);
            str_respLen += str_len;
        } else {
            break;
        }
    }

    httpd_resp_send_chunk(req, NULL, 0);               // Send empty data to indicate completion of transmission
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) // After sending the response, check if there are any previous request headers left. The ESP32 will clear them after sending.
    {
        ESP_LOGI(TAG, "Request headers lost");
    }
    heap_caps_free(resp_str);
    resp_str = NULL;
    return ESP_OK;
}

esp_err_t unknown_uri_handler(httpd_req_t *req) {
    const char *uri = req->uri; 
    ESP_LOGW("err", "Return directly URL:%s", uri);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Resources do not exist");
    return ESP_OK;
}

esp_err_t img_data_redirect_handler(httpd_req_t *req) {
    char       *buf        = (char *) heap_caps_malloc(READ_LEN_MAX + 1, MALLOC_CAP_SPIRAM);
    size_t      sdcard_len = 0;
    size_t      remaining  = req->content_len;
    const char *uri        = req->uri;
    size_t      ret;
    ESP_LOGI("TAG", "用户POST的URI是:%s,字节:%d", uri, remaining);
    xEventGroupSetBits(ServerPortGroups, (0x1UL << 0)); 
    SDPort_->SDPort_WriteOffset("/sdcard/02_sys_ap_img/user_send.bmp", NULL, 0, 0);
    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf, ServerPort_MIN(remaining, READ_LEN_MAX))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }
        size_t req_len = SDPort_->SDPort_WriteOffset("/sdcard/02_sys_ap_img/user_send.bmp", buf, ret, 1);
        sdcard_len += req_len; // Final comparison result
        remaining -= ret;      // Subtract the data that has already been received
    }
    xEventGroupSetBits(ServerPortGroups, (0x1UL << 1)); 
    if (sdcard_len == req->content_len) {
        httpd_resp_send_chunk(req, "上传成功", strlen("上传成功"));
        xEventGroupSetBits(ServerPortGroups, (0x1UL << 2));
    } 
    else {
        httpd_resp_send_chunk(req, "上传失败", strlen("上传失败"));
        xEventGroupSetBits(ServerPortGroups, (0x1UL << 3));
    } 
    httpd_resp_send_chunk(req, NULL, 0);

    heap_caps_free(buf);
    buf = NULL;
    return ESP_OK;
}

/*wifi sta */
static void wifi_event_callback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_id == WIFI_EVENT_STA_START) {
        ESP_ERROR_CHECK(esp_wifi_connect());
    } else if (event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(ServerPortGroups, GroupBit6); 
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGE("wifi", "WiFi disconnected, trying to reconnect...");
    }
}

static void mdns_init_config(void) {
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MDNS Init failed: %s", esp_err_to_name(err));
        return;
    }

    mdns_hostname_set("esp32-s3-photopainter");
    mdns_instance_name_set("ESP32-S3 WebServer");
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);

    ESP_LOGW(TAG, "mDns配置完成,可通过 http://esp32-s3-photopainter.local/index.html 访问");
}

/*html 代码*/
void ServerPort_init(CustomSDPort *SDPort) {
    if(SDPort_ == NULL) {
        SDPort_ = SDPort;
    }
    mdns_init_config();
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn   = httpd_uri_match_wildcard; /*Wildcard enabling*/
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    /*Event callback function*/
    httpd_uri_t uri_config = {};
    uri_config.uri         = "/index.html";
    uri_config.method      = HTTP_GET;
    uri_config.handler     = index_html_redirect_handler;
    uri_config.user_ctx    = NULL;
    httpd_register_uri_handler(server, &uri_config);

    uri_config.uri     = "/assets/bootstrap/css/bootstrap.min.css";
    uri_config.handler = bootstrap_css_redirect_handler;
    httpd_register_uri_handler(server, &uri_config);

    uri_config.uri     = "/assets/css/styles.min.css";
    uri_config.handler = styles_css_redirect_handler;
    httpd_register_uri_handler(server, &uri_config);

    uri_config.uri     = "/assets/js/script.min.js";
    uri_config.handler = script_js_redirect_handler;
    httpd_register_uri_handler(server, &uri_config);

    uri_config.uri     = "/assets/bootstrap/js/bootstrap.min.js";
    uri_config.handler = bootstrap_js_redirect_handler;
    httpd_register_uri_handler(server, &uri_config);

    uri_config.uri         = "/dataUP";
    uri_config.method      = HTTP_POST;
    uri_config.handler     = img_data_redirect_handler;
    uri_config.user_ctx    = NULL;
    httpd_register_uri_handler(server, &uri_config);

    uri_config.uri         = "/*"; // Match all URLs that have not been handled by other handlers
    uri_config.method      = HTTP_GET;
    uri_config.handler     = unknown_uri_handler; // Callback for returning a 404 response
    uri_config.user_ctx    = NULL;
    httpd_register_uri_handler(server, &uri_config);

    uri_config.uri         = "/*"; // Match all URLs that have not been handled by other handlers
    uri_config.method      = HTTP_POST;
    uri_config.handler     = unknown_uri_handler; // Callback for returning a 404 response
    uri_config.user_ctx    = NULL;
    httpd_register_uri_handler(server, &uri_config);
}

uint8_t ServerPort_NetworkInit(wifi_credential_t creden) {
    ServerPortGroups         = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    assert(esp_netif_create_default_wifi_sta());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t Instance_WIFI_IP;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_callback, NULL, &Instance_WIFI_IP);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_callback, NULL, &Instance_WIFI_IP);
    
    wifi_config_t wifi_config = {};
    strcpy((char *) wifi_config.sta.ssid, creden.ssid);
    strcpy((char *) wifi_config.sta.password, creden.password);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    EventBits_t even = xEventGroupWaitBits(ServerPortGroups, (GroupBit6), pdTRUE, pdFALSE, pdMS_TO_TICKS(8000));
    if(even & GroupBit6) {
        ESP_LOGW(TAG, "WiFi connected successfully");
        return 1;
    } else {
        ESP_LOGE(TAG, "WiFi connection timed out");
        return 0;
    }
}

void ServerPort_SetNetworkSleep(void) {
    esp_wifi_stop();
    esp_wifi_deinit();
    vTaskDelay(pdMS_TO_TICKS(500));
}