#include "netw_scan.h"
#include "sd_adapter.h"
#include "sniffer.h"

#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

#define MGMT_SSID "myssid"
#define MGMT_PWD  "mypassword"
#define MGMT_DEFAULT_SCAN_METHOD WIFI_FAST_SCAN
#define MGMT_DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#define MGMT_DEFAULT_RSSI -127
#define MGMT_DEFAULT_AUTHMODE WIFI_AUTH_OPEN

static const char *TAG = "scan";

static void print_auth_mode(FILE *F, int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        fprintf(F, "Authm = WIFI_AUTH_OPEN\n");
        break;
    case WIFI_AUTH_WEP:
        fprintf(F, "Authm = WIFI_AUTH_WEP\n");
        break;
    case WIFI_AUTH_WPA_PSK:
        fprintf(F, "Authm = WIFI_AUTH_WPA_PSK\n");
        break;
    case WIFI_AUTH_WPA2_PSK:
        fprintf(F, "Authm = WIFI_AUTH_WPA2_PSK\n");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        fprintf(F, "Authm = WIFI_AUTH_WPA_WPA2_PSK\n");
        break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
        fprintf(F, "Authm = WIFI_AUTH_WPA2_ENTERPRISE\n");
        break;
    case WIFI_AUTH_WPA3_PSK:
        fprintf(F, "Authm = WIFI_AUTH_WPA3_PSK\n");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        fprintf(F, "Authm = WIFI_AUTH_WPA2_WPA3_PSK\n");
        break;
    default:
        fprintf(F, "Authm = WIFI_AUTH_UNKNOWN\n");
        break;
    }
}

static void print_cipher_type(int pairwise_cipher, int group_cipher)
{
    switch (pairwise_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    default:
        ESP_LOGI(TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }

    switch (group_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    default:
        ESP_LOGI(TAG, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }
}

typedef struct {
    std::string file;
} scan_args;

void wifi_netw_scan_sta(wifi_scan_config_t *scan_cfg,  const std::string &filename) {

}

static std::string bssid2str(uint8_t *bssid) {
    char buffer[28] = {};
    snprintf(buffer, 28, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    return std::string{buffer};
}

void wifi_netw_scan_with_config(wifi_scan_config_t *scan_cfg, const std::string &filename) {
    uint16_t number = DEFAULT_SCAN_LIST_SIZE * 4;
    wifi_ap_record_t *ap_info = (wifi_ap_record_t *)malloc( number * sizeof(wifi_ap_record_t) );
    uint16_t ap_count = 0;
    scan_cfg->scan_time.active.min = 1000;
    scan_cfg->scan_time.active.max = 5000;
    scan_cfg->scan_type = WIFI_SCAN_TYPE_ACTIVE;

    std::string full_path = SNIFFER_MOUNT_POINT + filename + ".txt";
    ESP_LOGI(TAG, "Filename = %s", full_path.c_str());
    FILE *result = fopen(full_path.c_str(), "w+");
    if (!result) {
        ESP_LOGE(TAG, "Failed to open file");
        return;
    }
    memset(ap_info, 0, number * sizeof(wifi_ap_record_t));

    esp_wifi_scan_start(scan_cfg, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);

    if (scan_cfg)
        fprintf(result, "Channel %u\n", scan_cfg->channel);
    
    fprintf(result, "Total APs scanned = %u\n", ap_count);
    for (int i = 0; (i < number) && (i < ap_count); i++) {
        //ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        //ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        //print_auth_mode(stdout, ap_info[i].authmode);
        //if (ap_info[i].authmode != WIFI_AUTH_WEP) {
            //print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        //}
        //ESP_LOGI(TAG, "Channel \t\t%d\n", ap_info[i].primary);
        
        fprintf(result, "-----------------------------------------------\n");
        fprintf(result, "SSID  = %s\n", ap_info[i].ssid);
        fprintf(result, "BSSID = %s\n", bssid2str(ap_info[i].bssid).c_str());
        print_auth_mode(result, ap_info[i].authmode);
        fprintf(result, "RSSI  = %d\n", ap_info[i].rssi);
        fprintf(result, "Chan  = %d\n", ap_info[i].primary);
    }    
    fclose(result);
    free(ap_info);
}

void wifi_netw_scan(const std::string &filename) {
    wifi_scan_config_t default_scan_cfg;
    memset(&default_scan_cfg, 0, sizeof(default_scan_cfg));
    default_scan_cfg.scan_time.active.min = 1000;
    default_scan_cfg.scan_time.active.max = 5000;
    default_scan_cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    wifi_netw_scan_with_config(&default_scan_cfg, filename);
}
