idf_component_register(
  SRCS "led_bsp.c"
  PRIV_REQUIRES driver button_bsp
  INCLUDE_DIRS "./")
