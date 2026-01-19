#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <nvs.h>
#include <stdio.h>
#include <string.h>

#include "traverse_nvs.h"

TraverseNvs::TraverseNvs() {
}

TraverseNvs::~TraverseNvs() {
}

void TraverseNvs::TraverseNvs_PrintAllNvs(const char *ns_name) {
    nvs_iterator_t it = NULL;
    esp_err_t      err;

    // 1. 创建NVS迭代器（v5.x版本用法不变）
    err = nvs_entry_find(NVS_DEFAULT_PART_NAME, ns_name, NVS_TYPE_ANY, &it);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "NVS中未找到任何数据(命名空间：%s)", ns_name ? ns_name : "所有");
            return;
        }
        ESP_LOGE(TAG, "NVS迭代器创建失败: %s", esp_err_to_name(err));
        return;
    }

    // 2. 遍历所有NVS条目（适配v5.x的nvs_entry_next）
    nvs_entry_info_t info;
    int              entry_count = 0;
    while (it != NULL) {
        // 获取当前条目的信息（命名空间、键名、类型）
        nvs_entry_info(it, &info);
        entry_count++;

        // 打印基础信息
        ESP_LOGI(TAG, "\n===== 条目 %d =====", entry_count);
        ESP_LOGI(TAG, "命名空间: %s", info.namespace_name);
        ESP_LOGI(TAG, "键名: %s", info.key);

        // 根据类型读取并打印值
        nvs_handle_t handle;
        err = nvs_open(info.namespace_name, NVS_READONLY, &handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "打开命名空间失败: %s", esp_err_to_name(err));

            // 【关键修改1】v5.x：传&it（迭代器指针的指针）给nvs_entry_next
            err = nvs_entry_next(&it);
            if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
                ESP_LOGE(TAG, "迭代下一个条目失败: %s", esp_err_to_name(err));
            }
            continue;
        }

        switch (info.type) {
        case NVS_TYPE_U8: {
            uint8_t val;
            nvs_get_u8(handle, info.key, &val);
            ESP_LOGI(TAG, "类型: uint8_t | 值: %u", val);
            break;
        }
        case NVS_TYPE_I8: {
            int8_t val;
            nvs_get_i8(handle, info.key, &val);
            ESP_LOGI(TAG, "类型: int8_t | 值: %d", val);
            break;
        }
        case NVS_TYPE_U16: {
            uint16_t val;
            nvs_get_u16(handle, info.key, &val);
            ESP_LOGI(TAG, "类型: uint16_t | 值: %u", val);
            break;
        }
        case NVS_TYPE_I16: {
            int16_t val;
            nvs_get_i16(handle, info.key, &val);
            ESP_LOGI(TAG, "类型: int16_t | 值: %d", val);
            break;
        }
        case NVS_TYPE_U32: {
            uint32_t val;
            nvs_get_u32(handle, info.key, &val);
            ESP_LOGI(TAG, "命名空间: %s", info.namespace_name);
            ESP_LOGI(TAG, "类型: uint32_t | 值: %u", val);
            break;
        }
        case NVS_TYPE_I32: {
            int32_t val;
            nvs_get_i32(handle, info.key, &val);
            ESP_LOGI(TAG, "类型: int32_t | 值: %d", val);
            break;
        }
        case NVS_TYPE_U64: {
            uint64_t val;
            nvs_get_u64(handle, info.key, &val);
            ESP_LOGI(TAG, "类型: uint64_t | 值: %llu", val);
            break;
        }
        case NVS_TYPE_I64: {
            int64_t val;
            nvs_get_i64(handle, info.key, &val);
            ESP_LOGI(TAG, "类型: int64_t | 值: %lld", val);
            break;
        }
        case NVS_TYPE_STR: {
            size_t len;
            nvs_get_str(handle, info.key, NULL, &len); // 先获取字符串长度
            char *val = (char *) malloc(len);
            nvs_get_str(handle, info.key, val, &len);
            ESP_LOGI(TAG, "类型: string | 值: %s", val);
            free(val);
            break;
        }
        case NVS_TYPE_BLOB: {
            size_t len;
            nvs_get_blob(handle, info.key, NULL, &len); // 先获取二进制数据长度
            uint8_t *val = (uint8_t *) malloc(len);
            nvs_get_blob(handle, info.key, val, &len);
            ESP_LOGI(TAG, "类型: blob | 长度: %zu | 十六进制值: ", len);
            for (size_t i = 0; i < len; i++) {
                printf("%02x ", val[i]);
            }
            printf("\n");
            free(val);
            break;
        }
        default:
            ESP_LOGI(TAG, "类型: 未知 | 值: 无法读取");
            break;
        }

        nvs_close(handle);

        // 【关键修改2】v5.x：传&it（迭代器指针的指针）给nvs_entry_next
        err = nvs_entry_next(&it);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "迭代下一个条目失败: %s", esp_err_to_name(err));
            break;
        }
    }

    // 释放迭代器
    if (it != NULL) {
        nvs_release_iterator(it);
    }
    ESP_LOGI(TAG, "\n遍历完成,共找到 %d 个NVS条目", entry_count);
}

void TraverseNvs::parse_sta_ssid_blob(const uint8_t *blob_data, size_t blob_len, char *out_ssid) {
    if (blob_data == NULL || blob_len < 4 || out_ssid == NULL) {
        strcpy(out_ssid, "");
        return;
    }

    // 前4字节是小端序的SSID长度（uint32_t）
    uint32_t ssid_len = *((uint32_t *) blob_data);
    // SSID最大长度32，防止越界
    ssid_len = (ssid_len > 32) ? 32 : ssid_len;

    // 提取有效字符（跳过前4字节长度，取ssid_len个字符）
    memcpy(out_ssid, blob_data + 4, ssid_len);
    // 手动添加字符串结束符
    out_ssid[ssid_len] = '\0';

    ESP_LOGI(TAG, "解析到SSID: %s (长度: %u)", out_ssid, ssid_len);
}

void TraverseNvs::parse_sta_pswd_blob(const uint8_t *blob_data, size_t blob_len, char *out_password) {
    if (blob_data == NULL || blob_len == 0 || out_password == NULL) {
        strcpy(out_password, "");
        return;
    }

    // 提取有效ASCII字符（直到遇到0或达到最大长度）
    size_t pswd_len = 0;
    while (pswd_len < blob_len && pswd_len < 64) {
        if (blob_data[pswd_len] == '\0') {
            break;
        }
        out_password[pswd_len] = blob_data[pswd_len];
        pswd_len++;
    }
    // 添加字符串结束符
    out_password[pswd_len] = '\0';

    ESP_LOGI(TAG, "解析到密码: %s (长度: %zu)", out_password, pswd_len);
}

wifi_credential_t TraverseNvs::Get_WifiCredentialFromNVS(void) {
    wifi_credential_t cred = {0};
    cred.is_valid          = false;

    // 1. 打开nvs.net80211命名空间（只读模式）
    nvs_handle_t handle;
    esp_err_t    err = nvs_open("nvs.net80211", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "打开NVS命名空间nvs.net80211失败: %s", esp_err_to_name(err));
        return cred;
    }

    // 2. 读取sta.ssid的blob数据
    size_t ssid_blob_len = 0;
    // 第一步：获取blob数据长度
    err = nvs_get_blob(handle, "sta.ssid", NULL, &ssid_blob_len);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "NVS中未找到sta.ssid键");
        } else {
            ESP_LOGE(TAG, "读取sta.ssid长度失败: %s", esp_err_to_name(err));
        }
        nvs_close(handle);
        return cred;
    }

    // 第二步：分配内存并读取blob数据
    uint8_t *ssid_blob = (uint8_t *) malloc(ssid_blob_len);
    if (ssid_blob == NULL) {
        ESP_LOGE(TAG, "分配SSID blob内存失败");
        nvs_close(handle);
        return cred;
    }
    err = nvs_get_blob(handle, "sta.ssid", ssid_blob, &ssid_blob_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "读取sta.ssid blob数据失败: %s", esp_err_to_name(err));
        free(ssid_blob);
        nvs_close(handle);
        return cred;
    }
    // 解析SSID
    parse_sta_ssid_blob(ssid_blob, ssid_blob_len, cred.ssid);
    free(ssid_blob); // 释放临时内存

    // 3. 读取sta.pswd的blob数据
    size_t pswd_blob_len = 0;
    // 第一步：获取blob数据长度
    err = nvs_get_blob(handle, "sta.pswd", NULL, &pswd_blob_len);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "NVS中未找到sta.pswd键");
        } else {
            ESP_LOGE(TAG, "读取sta.pswd长度失败: %s", esp_err_to_name(err));
        }
        nvs_close(handle);
        return cred;
    }

    // 第二步：分配内存并读取blob数据
    uint8_t *pswd_blob = (uint8_t *) malloc(pswd_blob_len);
    if (pswd_blob == NULL) {
        ESP_LOGE(TAG, "分配密码blob内存失败");
        nvs_close(handle);
        return cred;
    }
    err = nvs_get_blob(handle, "sta.pswd", pswd_blob, &pswd_blob_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "读取sta.pswd blob数据失败: %s", esp_err_to_name(err));
        free(pswd_blob);
        nvs_close(handle);
        return cred;
    }
    // 解析密码
    parse_sta_pswd_blob(pswd_blob, pswd_blob_len, cred.password);
    free(pswd_blob); // 释放临时内存

    // 4. 关闭NVS句柄，标记数据有效
    nvs_close(handle);
    cred.is_valid = true;

    return cred;
}