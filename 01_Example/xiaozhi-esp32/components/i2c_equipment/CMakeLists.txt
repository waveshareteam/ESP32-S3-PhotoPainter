idf_component_register(
  SRCS "i2c_equipment.cpp"
  PRIV_REQUIRES i2c_bsp
  REQUIRES SensorLib
  INCLUDE_DIRS "./")
