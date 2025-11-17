#ifndef SDCARD_BSP_H
#define SDCARD_BSP_H

#include "driver/sdmmc_host.h"
#include "list.h"

typedef struct
{
    char sdcard_name[100];
	int name_score;       
}sdcard_node_t; 



extern sdmmc_card_t *card_host;
extern list_t *sdcard_scan_listhandle;

#ifdef __cplusplus
extern "C" {
#endif


uint8_t _sdcard_init(void);
void list_scan_dir(const char *path);
int list_iterator(void); 

void set_Currently_node(list_node_t *node);
list_node_t *get_Currently_node(void);

int sdcard_write_file(const char *path, const void *data, size_t data_len);
int sdcard_read_file(const char *path, uint8_t *buffer, size_t *outLen);
int sdcard_read_offset(const char *path, void *buffer, size_t len, size_t offset);
int sdcard_write_offset(const char *path, const void *data, size_t len, bool append);


#ifdef __cplusplus
}
#endif

#endif