set(src_dirs  
                  ./ 
                  ./Fonts)
set(include_dirs  
                  ./
                  ./Fonts)

idf_component_register(
  SRC_DIRS 
  ${src_dirs}
  PRIV_REQUIRES driver fatfs sdmmc sdcard_bsp
  INCLUDE_DIRS 
  ${include_dirs})
