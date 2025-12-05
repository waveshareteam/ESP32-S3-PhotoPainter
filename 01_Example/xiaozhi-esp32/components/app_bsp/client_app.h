#ifndef CLIENT_BSP_H
#define CLIENT_BSP_H


#ifdef __cplusplus
extern "C" {
#endif

const char *fetch_weather_data(void);

void auto_get_weather(void);
const char *auto_get_weather_json(void);

extern char province[64];
extern char city[64];


#ifdef __cplusplus
}
#endif

#endif
