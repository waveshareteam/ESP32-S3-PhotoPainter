#pragma once

#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <driver/sdmmc_host.h>
#include "list.h"


typedef struct
{
    char sdcard_name[50];  
}CustomSDPortNode_t;

class CustomSDPort
{
private:
    const char *TAG = "SDPort";
    const char *SdName_;
    int is_SdcardInitOK = 0;
    sdmmc_card_t *sdcard_host = NULL;
    list_t *ScanListHandle = NULL;

    list_node_t *CurrentlyNode = NULL; 
public:
    CustomSDPort(const char *SdName,int clk = 39,int cmd = 41,int d0 = 40,int d1 = 1,int d2 = 2,int d3 = 38,int width = 4);
    ~CustomSDPort();

    int SDPort_WriteFile(const char *path, const void *data, size_t data_len);
    int SDPort_ReadFile(const char *path, uint8_t *buffer, size_t *outLen);
    int SDPort_ReadOffset(const char *path, void *buffer, size_t len, size_t offset);
    int SDPort_WriteOffset(const char *path, const void *data, size_t len, bool append);
    sdmmc_card_t* SDPort_GetSdMMCHost();
    void SDPort_ScanListDir(const char *path);
    list_t* SDPort_GetListHost();
    int SDPort_GetSdcardInitOK();
    int SDPort_GetScanListValue(); 

    void SDPort_SetCurrentlyNode(list_node_t *node);
    list_node_t* SDPort_GetCurrentlyNode(void);
    uint16_t Get_Sdcard_ImgValue(void);
};