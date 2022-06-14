#ifndef _CUSTOM_SCAN_H
#define _CUSTOM_SCAN_H

#define DEFAULT_SCAN_LIST_SIZE 100
#define ALL_CHANNELS -1

#include "esp_wifi_types.h"

void wifi_netw_scan_with_config(wifi_scan_config_t *scan_cfg);
void wifi_netw_scan();
void wifi_netw_fast_scan();

#endif // _SCAN_H