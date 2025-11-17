idf_component_register(
  SRCS "json_data.cpp"
  REQUIRES i2c_equipment
  PRIV_REQUIRES sdcard_bsp
  INCLUDE_DIRS "./")
