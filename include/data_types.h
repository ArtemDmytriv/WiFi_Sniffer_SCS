#ifndef _DATA_TYPES_H
#define _DATA_TYPES_H

#include "esp_wifi_types.h"

// WLAN Sniffer Filter
typedef enum {
    SNIFFER_WLAN_FILTER_MGMT = 0, /*!< MGMT */
    SNIFFER_WLAN_FILTER_CTRL,     /*!< CTRL */
    SNIFFER_WLAN_FILTER_DATA,     /*!< DATA */
    SNIFFER_WLAN_FILTER_MISC,     /*!< MISC */
    SNIFFER_WLAN_FILTER_MPDU,     /*!< MPDU */
    SNIFFER_WLAN_FILTER_AMPDU,    /*!< AMPDU */
    SNIFFER_WLAN_FILTER_FCSFAIL,  /*!< When this bit is set, the hardware will receive packets for which frame check sequence failed */
    SNIFFER_WLAN_FILTER_MAX
} sniffer_wlan_filter_t;

typedef struct {
	unsigned frame_ctrl:16;
	unsigned duration_id:16;
	uint8_t addr1[6]; /* receiver address */
	uint8_t addr2[6]; /* sender address */
	uint8_t addr3[6]; /* filtering address */
	unsigned sequence_ctrl:16;
	uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
	wifi_ieee80211_mac_hdr_t hdr;
	uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

#endif