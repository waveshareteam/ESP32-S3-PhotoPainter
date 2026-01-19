#pragma once


typedef struct {
    char ssid[33];      // The maximum length of WiFi SSID is 32, with an additional 1 character for the end symbol.
    char password[65];  // The maximum length of WiFi password is 64, with an additional 1 character for the end symbol.
    bool is_valid;      // Indicates whether valid data has been successfully obtained.
} wifi_credential_t;

class TraverseNvs
{
private:
    const char *TAG = "NVS_VIEWER";

    void parse_sta_ssid_blob(const uint8_t *blob_data, size_t blob_len, char *out_ssid);
    void parse_sta_pswd_blob(const uint8_t *blob_data, size_t blob_len, char *out_password);
public:
    TraverseNvs();
    ~TraverseNvs();
    void TraverseNvs_PrintAllNvs(const char *ns_name);
    wifi_credential_t Get_WifiCredentialFromNVS(void);
};
