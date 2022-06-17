#ifndef _CUSTOM_SCAN_H
#define _CUSTOM_SCAN_H

#define DEFAULT_SCAN_LIST_SIZE 10
#define ALL_CHANNELS -1

#include "string"
#include "esp_wifi_types.h"

void wifi_netw_scan_with_config(wifi_scan_config_t *scan_cfg, const std::string &filename);
void wifi_netw_scan(const std::string &filename);

#endif // _SCAN_H