#ifndef USER_AUDIO_BSP_H
#define USER_AUDIO_BSP_H

#include "codec_board.h"
#include "codec_init.h"

class user_audio_bsp
{
private:
    esp_codec_dev_handle_t playback = NULL;
    esp_codec_dev_handle_t record = NULL;

public:
    user_audio_bsp();
    ~user_audio_bsp();
    uint8_t Play_InfoAudio();
    void Play_BackWrite(void *data_ptr,uint32_t len);
    uint8_t Close_Play();
    int Get_MusicSizt(uint8_t value);
    uint8_t* Get_MusicData(uint8_t value);
    void Set_CodecReg(const char * str,uint8_t reg,uint8_t data);
    uint8_t Get_CodecReg(const char *str, uint8_t reg);
};


#endif // !MY_ADF_BSP_H
