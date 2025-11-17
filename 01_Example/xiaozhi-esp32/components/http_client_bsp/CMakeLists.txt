idf_component_register(
  SRCS "client_bsp.c"
  PRIV_REQUIRES driver esp_http_client sdcard_bsp esp_http_server esp_wifi json
  INCLUDE_DIRS "."
)

target_add_binary_data(${COMPONENT_TARGET} "api_root_cert.pem" TEXT)