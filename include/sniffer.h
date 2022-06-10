#ifndef _SNIFFER_H
#define _SNIFFER_H

#include "data_types.h"

void wifi_sniffer_init();
void wifi_sniffer_set_channel(uint8_t channel);
const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);
void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);

#endif