#include "user_audio_bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "i2c_bsp.h"
#include <stdio.h>
#include <string.h>

#define SAMPLE_RATE 24000 // Sampling rate: 24000Hz
#define BIT_DEPTH 32      // Word size: 32 bits

esp_codec_dev_handle_t playback = NULL;
esp_codec_dev_handle_t record   = NULL;

extern const uint8_t mode_pcm_start[] asm("_binary_mode_pcm_start");
extern const uint8_t mode_pcm_end[] asm("_binary_mode_pcm_end");

extern const uint8_t one_pcm_start[] asm("_binary_mode_1_pcm_start");
extern const uint8_t one_pcm_end[] asm("_binary_mode_1_pcm_end");

extern const uint8_t two_pcm_start[] asm("_binary_mode_2_pcm_start");
extern const uint8_t two_pcm_end[] asm("_binary_mode_2_pcm_end");

extern const uint8_t three_pcm_start[] asm("_binary_mode_3_pcm_start");
extern const uint8_t three_pcm_end[] asm("_binary_mode_3_pcm_end");

user_audio_bsp::user_audio_bsp() {
    set_codec_board_type("USER_CODEC_BOARD");
    codec_init_cfg_t codec_cfg = {};
    codec_cfg.in_mode          = CODEC_I2S_MODE_TDM;
    codec_cfg.out_mode         = CODEC_I2S_MODE_TDM;
    codec_cfg.in_use_tdm       = false;
    codec_cfg.reuse_dev        = false;
    ESP_ERROR_CHECK(init_codec(&codec_cfg));
    playback = get_playback_handle();
    record   = get_record_handle();
}

user_audio_bsp::~user_audio_bsp() {
}

uint8_t user_audio_bsp::Play_InfoAudio() {
    esp_codec_dev_set_out_vol(playback, 100.0); //Set the volume to 100.
    esp_codec_dev_sample_info_t fs = {};
    fs.sample_rate                 = 16000;
    fs.channel                     = 2;
    fs.bits_per_sample             = 16;
    int     err                    = esp_codec_dev_open(playback, &fs); //Start playback
    uint8_t errx                   = (err == ESP_CODEC_DEV_OK) ? 1 : 0;
    return errx;
}

void user_audio_bsp::Play_BackWrite(void *data_ptr, uint32_t len) {
    esp_codec_dev_write(playback, data_ptr, len);
}

uint8_t user_audio_bsp::Close_Play() {
    int     err  = esp_codec_dev_close(playback);
    uint8_t errx = (err == ESP_CODEC_DEV_OK) ? 1 : 0;
    return errx;
}

int user_audio_bsp::Get_MusicSizt(uint8_t value) {
    if (value == 0)
        return (mode_pcm_end - mode_pcm_start);
    if (value == 1)
        return (one_pcm_end - one_pcm_start);
    if (value == 2)
        return (two_pcm_end - two_pcm_start);
    if (value == 3)
        return (three_pcm_end - three_pcm_start);
    return 0;
}

uint8_t *user_audio_bsp::Get_MusicData(uint8_t value) {
    uint8_t *ptr = NULL;
    if (value == 0)
        ptr = (uint8_t *) mode_pcm_start;
    if (value == 1)
        ptr = (uint8_t *) one_pcm_start;
    if (value == 2)
        ptr = (uint8_t *) two_pcm_start;
    if (value == 3)
        ptr = (uint8_t *) three_pcm_start;
    return ptr;
}

void user_audio_bsp::Set_CodecReg(const char *str, uint8_t reg, uint8_t data) {
    if (!strcmp(str, "es8311"))
        i2c_write_buff(es8311_dev_handle, reg, &data, 1);
    if (!strcmp(str, "es7210"))
        i2c_write_buff(es7210_dev_handle, reg, &data, 1);
}

uint8_t user_audio_bsp::Get_CodecReg(const char *str, uint8_t reg) {
    uint8_t data = 0x00;
    if (!strcmp(str, "es8311"))
        i2c_read_buff(es8311_dev_handle, reg, &data, 1);
    if (!strcmp(str, "es7210"))
        i2c_read_buff(es7210_dev_handle, reg, &data, 1);
    return data;
}

/*
Board: AMOLED_1_43
i2c: {sda: 18, scl: 8}
i2s: {bclk: 21, ws: 22, dout: 23, din: 20, mclk: 19}
out: {codec: ES8311, pa: -1, use_mclk: 1, pa_gain:6}
in: {codec: ES7210}
*/