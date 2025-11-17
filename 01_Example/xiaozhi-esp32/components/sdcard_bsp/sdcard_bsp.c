#include "sdcard_bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

static const char *TAG = "_sdcard";

#define SDMMC_D0_PIN 40
#define SDMMC_D1_PIN 1
#define SDMMC_D2_PIN 2
#define SDMMC_D3_PIN 38
#define SDMMC_CLK_PIN 39
#define SDMMC_CMD_PIN 41

#define SDlist "/sdcard" 

sdmmc_card_t *card_host = NULL;

list_t *sdcard_scan_listhandle = NULL;

static list_node_t *Currently_node = NULL; 

uint8_t _sdcard_init(void) {
    sdcard_scan_listhandle = list_new();
    esp_vfs_fat_sdmmc_mount_config_t mount_config =
        {
            .format_if_mount_failed = false,         
            .max_files              = 5,             
            .allocation_unit_size   = 16 * 1024 * 3, 
        };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width               = 4; 
    slot_config.clk                 = SDMMC_CLK_PIN;
    slot_config.cmd                 = SDMMC_CMD_PIN;
    slot_config.d0                  = SDMMC_D0_PIN;
    slot_config.d1                  = SDMMC_D1_PIN;
    slot_config.d2                  = SDMMC_D2_PIN;
    slot_config.d3                  = SDMMC_D3_PIN;

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_vfs_fat_sdmmc_mount(SDlist, &host, &slot_config, &mount_config, &card_host));

    if (card_host != NULL) {
        sdmmc_card_print_info(stdout, card_host);
        return 1;
    }
    return 0;
}

/**
* @brief Write binary file to SD card
* @param path      File path
* @param data      Pointer to the data to be written
* @param data_len  Length of the data (in bytes)
*/
int sdcard_write_file(const char *path, const void *data, size_t data_len) {
    if (card_host == NULL) {
        ESP_LOGE(TAG, "SD card not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (sdmmc_get_status(card_host) != ESP_OK) {
        ESP_LOGE(TAG, "SD card not ready");
        return ESP_FAIL;
    }

    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", path);
        return ESP_ERR_NOT_FOUND;
    }

    size_t written = fwrite(data, 1, data_len, f);
    fclose(f);

    if (written != data_len) {
        ESP_LOGE(TAG, "Write failed (%zu/%zu bytes)", written, data_len);
        return ESP_FAIL;
    }

    //ESP_LOGI(TAG, "File written: %s (%zu bytes)", path, data_len);
    return ESP_OK;
}

/**
* @brief Read all data from the file
* @param path   File path
* @param buffer Buffer to store the read data
* @param outLen Number of bytes actually read 
*/
int sdcard_read_file(const char *path, uint8_t *buffer, size_t *outLen) {
    if (card_host == NULL) {
        ESP_LOGE(TAG, "SD card not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (sdmmc_get_status(card_host) != ESP_OK) {
        ESP_LOGE(TAG, "SD card not ready");
        return ESP_FAIL;
    }

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
        return ESP_ERR_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    if (file_size <= 0) {
        ESP_LOGE(TAG, "Invalid file size");
        fclose(f);
        return ESP_FAIL;
    }
    fseek(f, 0, SEEK_SET);

    size_t bytes_read = fread(buffer, 1, file_size, f);
    fclose(f);

    if (outLen) *outLen = bytes_read;

    //ESP_LOGI(TAG, "Read %zu/%ld bytes from %s", bytes_read, file_size, path);
    return (bytes_read > 0) ? ESP_OK : ESP_FAIL;
}

/**
* @brief Read data from the specified offset in the file
* @param path   File path
* @param buffer Buffer to store the data
* @param len    Length to read
* @param offset Offset position
* @param outLen Actual read length (can be NULL) 
*/
int sdcard_read_offset(const char *path, void *buffer, size_t len, size_t offset) {
    if (card_host == NULL) {
        ESP_LOGE(TAG, "SD card not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (sdmmc_get_status(card_host) != ESP_OK) {
        ESP_LOGE(TAG, "SD card not ready");
        return ESP_FAIL;
    }

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
        return ESP_ERR_NOT_FOUND;
    }

    fseek(f, offset, SEEK_SET);
    size_t bytes_read = fread(buffer, 1, len, f);
    fclose(f);

    //ESP_LOGI(TAG, "Read %zu bytes from %s (offset=%zu)", bytes_read, path, offset);

    return bytes_read;
}

/**
* @brief Writes data to the specified position of the file (supports clearing mode)
* @param path  File path
* @param data  Data pointer
* @param len   Data length
* @param append Whether it is an append mode (true = append, false = clear and rewrite)
*/
int sdcard_write_offset(const char *path, const void *data, size_t len, bool append) {
    if (card_host == NULL) {
        ESP_LOGE(TAG, "SD card not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (sdmmc_get_status(card_host) != ESP_OK) {
        ESP_LOGE(TAG, "SD card not ready");
        return ESP_FAIL;
    }

    const char *mode = append ? "ab" : "wb";
    FILE *f = fopen(path, mode);
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", path);
        return ESP_ERR_NOT_FOUND;
    }

    size_t bytes_written = fwrite(data, 1, len, f);
    fclose(f);

    if (!append && len == 0) {
        ESP_LOGI(TAG, "File cleared: %s", path);
        return ESP_OK;
    }

    //ESP_LOGI(TAG, "Wrote %zu bytes to %s (append=%d)", bytes_written, path, append);
    return bytes_written;
}

void list_scan_dir(const char *path) {
    struct dirent *entry;
    DIR           *dir = opendir(path);

    if (dir == NULL) {
        ESP_LOGE("sdscan", "Failed to open directory: %s", path);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) { 
            ESP_LOGI("sdscan", "Directory: %s", entry->d_name);
        } else {
            if (strstr(entry->d_name, ".bmp") == NULL) {
                continue;
            }
            uint16_t       _strlen   = strlen(path) + strlen(entry->d_name) + 1 + 1; 
            sdcard_node_t *node_data = (sdcard_node_t *) LIST_MALLOC(sizeof(sdcard_node_t));
            assert(node_data);
            if (_strlen > 96) {
                ESP_LOGE("sdcard", "scan file fill _strlen:%d", _strlen);
                continue;
            }
            node_data->name_score = 0;
            snprintf(node_data->sdcard_name, sizeof(node_data->sdcard_name) - 2, "%s/%s", path, entry->d_name); 
            list_rpush(sdcard_scan_listhandle, list_node_new(node_data));                                       
        }
    }
    closedir(dir);
}

int list_iterator(void) 
{
    int              Quantity = 0;
    list_iterator_t *it       = list_iterator_new(sdcard_scan_listhandle, LIST_HEAD); 
    list_node_t     *node     = list_iterator_next(it);
    while (node != NULL) {
        sdcard_node_t *sdcard_node = (sdcard_node_t *) node->val;
        ESP_LOGI("sdscan", "File: %s", sdcard_node->sdcard_name);
        node = list_iterator_next(it);
        Quantity++;
    }
    list_iterator_destroy(it); 
    return Quantity;
}

void set_Currently_node(list_node_t *node) {
    Currently_node = node;
}

list_node_t *get_Currently_node(void) {
    return Currently_node;
}
