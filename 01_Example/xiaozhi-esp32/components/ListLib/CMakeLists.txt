idf_component_register(
  SRCS "./src/list_iterator.c" "./src/list.c" "./src/list_node.c"
  PRIV_REQUIRES driver
  INCLUDE_DIRS "./include/")
