#ifndef _SNIFFER_H
#define _SNIFFER_H

#include "data_types.h"
#include <vector>
#include <string>

typedef struct {
    std::vector<std::string> filters;
    int channel;
    int stop;
    int number;
    bool is_sta_sniff_mode;
    uint8_t bssid[6];
} sniffer_args_t;

extern FILE *f_to_write;

int do_sniffer_cmd(sniffer_args_t* args_sniff );
void create_wifi_filter_hashtable(void);

void wifi_sniffer_set_channel(uint8_t channel);
const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);
void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);
esp_err_t  sniff_packet_stop(void);

#endif