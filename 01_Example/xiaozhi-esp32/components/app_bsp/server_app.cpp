#include <stdio.h>
#include <esp_check.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include "server_app.h"
#include "sdcard_bsp.h"

static const char *TAG = "server_bsp";

#define ServerPort_MIN(x, y) ((x < y) ? (x) : (y))
#define READ_LEN_MAX (10 * 1024) // Buffer area for receiving data
#define SEND_LEN_MAX (5 * 1024)  // Data for sending response

#define EXAMPLE_ESP_WIFI_SSID "esp_network"
#define EXAMPLE_ESP_WIFI_PASS "1234567890"
#define EXAMPLE_ESP_WIFI_CHANNEL 1
#define EXAMPLE_MAX_STA_CONN 4

EventGroupHandle_t ServerPortGroups;
static CustomSDPort *SDPort_ = NULL;

/*Callback function*/
esp_err_t get_html_callback(httpd_req_t *req);
esp_err_t get_css_callback(httpd_req_t *req);
esp_err_t get_js_callback(httpd_req_t *req);
esp_err_t post_dataup_callback(httpd_req_t *req);
esp_err_t get_404_callback(httpd_req_t *req);

/*The callback function for handling GET requests*/
esp_err_t get_html_callback(httpd_req_t *req) {
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

esp_err_t get_css_callback(httpd_req_t *req) {
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
    char   Name_str[125] = {""};
    snprintf(Name_str, sizeof(Name_str), "/sdcard/03_sys_ap_html%.100s", uri);
    httpd_resp_set_type(req, "text/css");
    while (1) {
        str_len = SDPort_->SDPort_ReadOffset(Name_str, resp_str, SEND_LEN_MAX, str_respLen);
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

esp_err_t get_js_callback(httpd_req_t *req) {
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
    char   Name_str[125] = {""};
    snprintf(Name_str, sizeof(Name_str), "/sdcard/03_sys_ap_html%.100s", uri);
    httpd_resp_set_type(req, "application/javascript");
    while (1) {
        str_len = SDPort_->SDPort_ReadOffset(Name_str, resp_str, SEND_LEN_MAX, str_respLen);
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

esp_err_t get_404_callback(httpd_req_t *req) {
    const char *uri = req->uri; 
    ESP_LOGI("err", "Return directly URL:%s", uri);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Resources do not exist");
    return ESP_OK;
}

esp_err_t post_dataup_callback(httpd_req_t *req) {
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

/*wifi ap init*/
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        xEventGroupSetBits(ServerPortGroups, (0x01UL << 4));
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        xEventGroupSetBits(ServerPortGroups, (0x01UL << 5));
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED) {
    }
}

/*html 代码*/
void ServerPort_init(CustomSDPort *SDPort) {
    if(SDPort_ == NULL) {
        SDPort_ = SDPort;
    }
    ServerPortGroups         = xEventGroupCreate();
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn   = httpd_uri_match_wildcard; /*Wildcard enabling*/
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    /*Event callback function*/
    httpd_uri_t uri_get = {};
    uri_get.method      = HTTP_GET;
    uri_get.user_ctx    = NULL;
    uri_get.uri         = "/index.html";
    uri_get.handler     = get_html_callback;
    httpd_register_uri_handler(server, &uri_get);

    uri_get.uri     = "/assets/bootstrap/css/bootstrap.min.css";
    uri_get.handler = get_css_callback;
    httpd_register_uri_handler(server, &uri_get);

    uri_get.uri     = "/assets/css/styles.min.css";
    uri_get.handler = get_css_callback;
    httpd_register_uri_handler(server, &uri_get);

    uri_get.uri     = "/assets/js/script.min.js";
    uri_get.handler = get_js_callback;
    httpd_register_uri_handler(server, &uri_get);

    uri_get.uri     = "/assets/bootstrap/js/bootstrap.min.js";
    uri_get.handler = get_js_callback;
    httpd_register_uri_handler(server, &uri_get);

    httpd_uri_t uri_post = {};
    uri_post.uri         = "/dataUP";
    uri_post.method      = HTTP_POST;
    uri_post.handler     = post_dataup_callback;
    uri_post.user_ctx    = NULL;
    httpd_register_uri_handler(server, &uri_post);

    httpd_uri_t uri_404 = {};
    uri_404.uri         = "/*"; // Match all URLs that have not been handled by other handlers
    uri_404.method      = HTTP_GET;
    uri_404.handler     = get_404_callback; // Callback for returning a 404 response
    uri_404.user_ctx    = NULL;
    httpd_register_uri_handler(server, &uri_404);
}

void ServerPort_NetworkInit(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    assert(esp_netif_create_default_wifi_ap());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_AP_STAIPASSIGNED,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {};
    snprintf((char *) wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), "%s", EXAMPLE_ESP_WIFI_SSID);
    snprintf((char *) wifi_config.ap.password, sizeof(wifi_config.ap.password), "%s", EXAMPLE_ESP_WIFI_PASS);
    wifi_config.ap.channel        = EXAMPLE_ESP_WIFI_CHANNEL;
    wifi_config.ap.max_connection = EXAMPLE_MAX_STA_CONN;
    wifi_config.ap.authmode       = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI("network", "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

void ServerPort_SetNetworkSleep(void) {
    esp_wifi_stop();
    esp_wifi_deinit();
    vTaskDelay(pdMS_TO_TICKS(500));
}