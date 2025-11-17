idf_component_register(
    SRCS "esp32_ai_bsp.cpp" "floyd_steinberg.cpp" "./jpg_src/test_decoder.c" "./jpg_src/image_io.c"
    PRIV_REQUIRES sdcard_bsp driver json_bsp espressif__esp_new_jpeg fatfs espressif__esp_jpeg
    REQUIRES esp_http_client
    INCLUDE_DIRS "./" "./jpg_src"
    EMBED_TXTFILES 
    "./certs/ark_vol.pem"
    "./certs/volces_chain.pem")
