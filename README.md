
# ESP32-S3-PhotoPainter

- 中文wiki链接: https://www.waveshare.net/wiki/ESP32-S3-PhotoPainter
- Product English wiki link: https://www.waveshare.com/wiki/ESP32-S3-PhotoPainter

- 固件烧录说明链接: [前往点击展开浏览](https://www.waveshare.net/wiki/ESP32-S3-PhotoPainter#.E7.83.A7.E5.BD.95.E5.9B.BA.E4.BB.B6.E8.AF.B4.E6.98.8E)
- Firmware burning instructions link: [Click here to expand and view](https://www.waveshare.com/wiki/ESP32-S3-PhotoPainter#Firmware_Flashing_Instructions)

## Change log

### [1.2.0] 2026/01/30
- Supports JPG, PNG, and BMP image formats natively; no longer requires conversion via specific software for display.
- Currently, the maximum supported resolution for Mode 1 and Mode 3 is 1350×1350. JPG images only support Baseline DCT encoding. There are no specific requirements for Mode 2, and all decoding jitter is implemented on the web side.
- Mode 2 now supports switching between STA and AP modes, no longer limited to AP mode only. The general usage workflow is as follows:
    - AP Mode (Default)
        - The device defaults to AP Mode. Connect to the WiFi with the following credentials: SSID: esp_network, Password: 1234567890.
        - Access the web interface via browser: http://192.168.4.1/index.html.
    - STA Mode
        - In AP Mode, configure the device to switch to STA Mode via the web interface, then restart the device to enter STA Mode.
        - If Mode 3 has not been configured with network settings, network configuration is required to use STA Mode (as STA Mode shares the same connected router as Mode 3).
        - The PWR LED blinks 5 times to indicate that no router information is stored in the current system.
        - There was no abnormal flashing of the PWR, and it is accessible:http://esp32-s3-photopainter.local/index.html

### [1.1.0] 2025/12/20
- In Mode 3, the scoring mechanism has been deprecated, and voice control capability has been newly added, which supports triggering the entry/exit operations of carousel images via voice commands, while also enabling dynamic adjustment of carousel interval duration through voice instructions;
- Fixed the abnormal issue where the device continuously restarts due to failed acquisition of weather data after the completion of network provisioning;
- Fixed the data overflow issue occurring in the interval wake-up process under Mode 1, upgraded the data type of the interval duration parameter from uint32_t to uint64_t to mitigate the risk of timer value overflow;
- Expanded the storage cache capacity for TF card file names to 80 bytes to accommodate scenarios with longer file naming conventions;
- Fixed the display anomaly where the device screen presents a blank screen when the TF card image reading operation fails.

## 更新日志

### [1.2.0] 2026/01/30
- 兼容JPG,PNG,BMP图片,不再需要经过特定的软件转换才能显示
- 目前模式1和模式3支持的分辨率最大为1350x1350,JPG格式的图片仅支持Baseline DCT编码,模式2没有特定的要求,解码抖动均在网页端实现
- 模式2可以实现STA和AP切换,不再局限于AP模式,大致使用流程是:
    - AP模式(默认)
        - 默认是AP模式,通过连接ssid:esp_network,psw:1234567890
        - 浏览器访问:http://192.168.4.1/index.html
    - STA模式
        - 在AP模式下,通过Web设置成STA模式,重启即可进入STA模式
        - 如果模式3没有配网,需要配网才能使用STA模式,因为STA的连接的路由器和模式3共存
        - PWR灯闪烁5次即表示当前系统没有路由器信息
        - PWR没有异常闪烁,可以访问:http://esp32-s3-photopainter.local/index.html

### [1.1.0] 2025/12/20
- 模式 3 中废弃打分机制，新增语音控制能力，支持通过语音指令触发轮播图片的进入/退出操作，同时支持语音指令动态调整轮播间隔时长；
- 修复配网完成后，因天气数据获取失败导致设备持续重启的异常问题；
- 修复模式 1 下间隔唤醒流程中出现的数据溢出问题，将间隔时长参数的数据类型从 uint32_t 升级为 uint64_t，规避定时器数值溢出风险；
- 将 TF 卡文件名称存储缓存的容量扩容至 80 字节，适配更长的文件命名场景；
- 修复 TF 卡图片读取操作失败时，设备屏幕出现白屏的显示异常问题。
