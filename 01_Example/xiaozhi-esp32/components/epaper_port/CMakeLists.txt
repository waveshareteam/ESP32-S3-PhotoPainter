idf_component_register(
  SRCS "epaper_port.c"
  PRIV_REQUIRES driver fatfs sdmmc sdcard_bsp
  INCLUDE_DIRS "./")
