#pragma once

#include "codec_board.h"
#include "codec_init.h"
#include "i2c_bsp.h"

class CodecPort
{
private:
    esp_codec_dev_handle_t playback = NULL;
    esp_codec_dev_handle_t record = NULL;
    I2cMasterBus& i2cbus_;
    i2c_master_dev_handle_t I2c_DevEs8311;
    i2c_master_dev_handle_t I2c_DevEs7210;
    const uint8_t Es8311Address = 0x18;
    const uint8_t Es7210Address = 0x40;

public:
    CodecPort(I2cMasterBus& i2cbus);
    ~CodecPort();
    uint8_t Codec_PlayInfoAudio();
    void Codec_PlayBackWrite(void *data_ptr,uint32_t len);
    uint8_t Codec_ClosePlay();
    int Codec_GetMusicSizt(uint8_t value);
    uint8_t* Codec_GetMusicData(uint8_t value);
    void Codec_SetCodecReg(const char * str,uint8_t reg,uint8_t data);
    uint8_t Codec_GetCodecReg(const char *str, uint8_t reg);
};

