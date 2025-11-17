idf_component_register(
  SRCS "sdcard_bsp.c"
  PRIV_REQUIRES 
  fatfs 
  REQUIRES
  ListLib 
  esp_driver_sdmmc   
  INCLUDE_DIRS "./")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format-truncation)
#REQUIRES fatfs
#PRIV_REQUIRES