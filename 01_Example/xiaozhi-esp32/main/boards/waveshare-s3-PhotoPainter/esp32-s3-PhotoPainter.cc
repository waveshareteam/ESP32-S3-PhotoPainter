#include "application.h"
#include "button.h"
#include "codecs/box_audio_codec.h"
#include "config.h"
#include "wifi_board.h"
#include <wifi_station.h>

#include "power_save_timer.h"
#include "user_app.h"
#include <driver/i2c_master.h>
#include <esp_log.h>

#include "mcp_server.h"

#define TAG "esp-s3-PhotoPainter"

class waveshare_PhotoPainter : public WifiBoard {
  private:
    i2c_master_bus_handle_t codec_i2c_bus_;
    Button                  boot_button_;
    PowerSaveTimer         *power_save_timer_;

    void InitializeCodecI2c() {
        ESP_ERROR_CHECK(i2c_master_get_bus_handle(0, &codec_i2c_bus_));
    }

    //void InitializePowerSaveTimer() {
    //    power_save_timer_ = new PowerSaveTimer(-1, 60, -1);
    //    power_save_timer_->OnEnterSleepMode([this]() { //sleep
    //        ESP_LOGE("power", "Fall asleep");
    //        auto& app = Application::GetInstance();
    //        app.ToggleChatState();
    //        app.ToggleChatState();
    //        gpio_set_level((gpio_num_t) 45, 1);
    //    });
    //    power_save_timer_->OnExitSleepMode([this]() { //Exit sleep
    //        ESP_LOGE("power", "Exit sleep");
    //        gpio_set_level((gpio_num_t) 45, 0);
    //    });
    //    power_save_timer_->OnShutdownRequest([this]() { //Power off
    //        ESP_LOGE("power", "Power off");
    //        auto& app = Application::GetInstance();
    //        app.ToggleChatState();
    //        app.ToggleChatState();
    //    });
    //    power_save_timer_->SetEnabled(true); //Enable the timer
    //}

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto &app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });
    }

    void InitializeTools() {
        auto &mcp_server = McpServer::GetInstance();
        mcp_server.AddTool("self.disp.SwitchPictures", "切换本地或 SD 卡中的图片，通过整数参数指定图片序号（如 “显示第 1 张图片”）", PropertyList({Property("value", kPropertyTypeInteger, 1, sdcard_bmp_Quantity)}), [this](const PropertyList &properties) -> ReturnValue {
            int value = properties["value"].value<int>();
            ESP_LOGE("vlaue", "%d", value);
            sdcard_doc_count = value;
            xEventGroupClearBits(ai_IMG_Score_Group, 0x01); //There is no need to poll for photos anymore.
            xEventGroupSetBits(epaper_groups, 0x02);        //  0000  0010
            return true;
        });

        mcp_server.AddTool("self.disp.getNumberimages", "获取 SD 卡中存储的图片文件总数，无输入参数，返回整数类型的图片数量", PropertyList(), [this](const PropertyList &) -> ReturnValue {
            xEventGroupSetBits(ai_IMG_Group, 0x02); //Retrieve the images from the SD card
            if (xSemaphoreTake(ai_img_while_semap, pdMS_TO_TICKS(2000)) == pdTRUE) {
                return sdcard_bmp_Quantity;
            } else {
                return false;
            }
        });

        mcp_server.AddTool("self.disp.aiIMG", "这个是用户可以根据语音生成图片的(图片生成大概需要10-20s时间),比如：帮我生成一张动漫图片,直接生成就好，不要回复乱七八糟的东西", PropertyList(), [this](const PropertyList &) -> ReturnValue {
            ESP_LOGI("MCP", "进入MCP aiIMG");
            if (!is_ai_img) { //Indicates that the image is being refreshed
                ESP_LOGE("MCP", "is_ai_img fill %d", is_ai_img);
                return false;
            }
            xEventGroupClearBits(ai_IMG_Score_Group, 0x01); //There is no need to poll for photos anymore.
            xEventGroupSetBits(ai_IMG_Group, 0x01);         //Indicates that access to the volcano is permitted to obtain IMG.
            return true;
        });

        mcp_server.AddTool("self.disp.Score", "对当前显示的图片进行评分，支持整数分数（如 “打 5 分”）或语义评价（如 “非常好看”“不好看”），输入参数为评分值或评价文本，用于记录图片评分数据", PropertyList({Property("value", kPropertyTypeInteger, 0, 5)}), [this](const PropertyList &properties) -> ReturnValue {
            ESP_LOGI("MCP", "进入MCP Score");
            IMG_Score = properties["value"].value<int>();
            xEventGroupSetBits(ai_IMG_Group, 0x04); //Assign a score
            if (xSemaphoreTake(ai_img_while_semap, pdMS_TO_TICKS(2000)) == pdTRUE) {
                xEventGroupClearBits(ai_IMG_Score_Group, 0x01); //There is no need to poll for photos anymore.
                return true;
            }
            return false;
        });

        mcp_server.AddTool("self.disp.lunScore", "启动高分图片轮询播放模式，自动筛选评分高的图片并循环展示，无参数，持续播放直到手动停止", PropertyList(), [this](const PropertyList &) -> ReturnValue {
            ESP_LOGI("MCP", "进入MCP lunScore");
            xEventGroupSetBits(ai_IMG_Score_Group, 0x01); //Poll to display high-quality images
            return true;
        });

        mcp_server.AddTool("self.disp.resetScore", "将所有图片的评分数据重置为初始状态，无参数，清除历史评分记录", PropertyList(), [this](const PropertyList &) -> ReturnValue {
            ESP_LOGI("MCP", "进入MCP resetScore");
            xEventGroupClearBits(ai_IMG_Score_Group, 0x01); //There is no need to poll for photos anymore.
            xEventGroupSetBits(ai_IMG_Score_Group, 0x02);   //Reset the score
            return true;
        });

        mcp_server.AddTool("self.disp.isSLeep", "使设备进入低功耗睡眠模式，关闭显示等非必要功能以节省电量，无参数，执行后设备进入休眠状态", PropertyList(), [this](const PropertyList &) -> ReturnValue {
            ESP_LOGI("MCP", "进入MCP isSLeep");
            xEventGroupSetBits(ai_IMG_Group, 0x08); //Low-power mode
            return true;
        });
    }

  public:
    waveshare_PhotoPainter()
        : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeCodecI2c();
        //InitializePowerSaveTimer();
        User_xiaozhi_app_init();
        InitializeButtons();
        InitializeTools();
    }

    virtual AudioCodec *GetAudioCodec() override {
        static BoxAudioCodec audio_codec(
            codec_i2c_bus_,
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK,
            AUDIO_I2S_GPIO_BCLK,
            AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT,
            AUDIO_I2S_GPIO_DIN,
            AUDIO_CODEC_PA_PIN,
            AUDIO_CODEC_ES8311_ADDR,
            AUDIO_CODEC_ES7210_ADDR,
            AUDIO_INPUT_REFERENCE);
        return &audio_codec;
    }

    //virtual void SetPowerSaveMode(bool enabled) override {
    //    if (!enabled) {
    //        power_save_timer_->WakeUp();
    //    }
    //    WifiBoard::SetPowerSaveMode(enabled);
    //}
};

DECLARE_BOARD(waveshare_PhotoPainter);

/*
afe_config->agc_init = false;
*/