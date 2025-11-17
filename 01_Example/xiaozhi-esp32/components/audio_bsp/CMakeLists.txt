idf_component_register(
    SRCS "user_audio_bsp.cpp" 
    PRIV_REQUIRES 
    espressif__esp_codec_dev
    i2c_bsp
    REQUIRES
    codec_board
    INCLUDE_DIRS "./"
    EMBED_FILES 
    "pcm_src/mode.pcm"
    "pcm_src/mode_1.pcm"
    "pcm_src/mode_2.pcm"
    "pcm_src/mode_3.pcm")