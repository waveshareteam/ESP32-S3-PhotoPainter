idf_component_register(
  SRCS 
  "mode_src/xiaozhi_mode.cpp" 
  "mode_src/Network_mode.cpp"
  "mode_src/Basic_mode.cpp" 
  "mode_src/Mode_Selection.cpp"
  "user_app.cpp" 
  PRIV_REQUIRES 
  driver        
  nvs_flash
  esp_wifi
  main
  app_bsp
  codec_board
  pmicpower
  port_bsp
  INCLUDE_DIRS "./")
