#include "sniffer.h"
#include "sd_adapter.h"

#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"

void wifi_sniffer_init() {
	esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

void wifi_sniffer_start() {
	esp_wifi_set_promiscuous(true);
}

void wifi_sniffer_stop() {
	esp_wifi_set_promiscuous(false);
}

void wifi_sniffer_set_channel(uint8_t channel) {
	esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

const char* wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type) {
	switch(type) {
        case WIFI_PKT_MGMT: return "MGMT";
        case WIFI_PKT_DATA: return "DATA";
        case WIFI_PKT_CTRL: return "CTRL";
	default:	
	    case WIFI_PKT_MISC: return "MISC";
	}
}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {

	if (type != WIFI_PKT_MGMT)
		return;

	const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
	const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
	const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

	// printf("PACKET TYPE=%s, CHAN=%02d, RSSI=%02d,"
	// 	" ADDR1=%02x:%02x:%02x:%02x:%02x:%02x,"
	// 	" ADDR2=%02x:%02x:%02x:%02x:%02x:%02x,"
	// 	" ADDR3=%02x:%02x:%02x:%02x:%02x:%02x\n",
	// 	wifi_sniffer_packet_type2str(type),
	// 	ppkt->rx_ctrl.channel,
	// 	ppkt->rx_ctrl.rssi,
	// 	/* ADDR1 */
	// 	hdr->addr1[0],hdr->addr1[1],hdr->addr1[2],
	// 	hdr->addr1[3],hdr->addr1[4],hdr->addr1[5],
	// 	/* ADDR2 */
	// 	hdr->addr2[0],hdr->addr2[1],hdr->addr2[2],
	// 	hdr->addr2[3],hdr->addr2[4],hdr->addr2[5],
	// 	/* ADDR3 */
	// 	hdr->addr3[0],hdr->addr3[1],hdr->addr3[2],
	// 	hdr->addr3[3],hdr->addr3[4],hdr->addr3[5]
	// );
}

//??

#define SNIFFER_DEFAULT_CHANNEL             (1)
#define SNIFFER_PAYLOAD_FCS_LEN             (4)
#define SNIFFER_PROCESS_PACKET_TIMEOUT_MS   (100)
#define SNIFFER_RX_FCS_ERR                  (0X41)
#define SNIFFER_MAX_ETH_INTFS               (3)
#define SNIFFER_DECIMAL_NUM                 (10)

#define SNIFFER_WORK_QUEUE_LEN 128
#define SNIFFER_TASK_STACK_SIZE 4096
#define SNIFFER_TASK_PRIORITY 2

static const char *SNIFFER_TAG = "cmd_sniffer";

typedef struct {
    char *filter_name;
    uint32_t filter_val;
} wlan_filter_table_t;

typedef struct {
    bool is_running;
    sniffer_intf_t interf;
    uint32_t interf_num;
    uint32_t channel;
    uint32_t filter;
    int32_t packets_to_sniff;
    TaskHandle_t task;
    QueueHandle_t work_queue;
    SemaphoreHandle_t sem_task_over;
    esp_eth_handle_t eth_handles[SNIFFER_MAX_ETH_INTFS];
} sniffer_runtime_t;

typedef struct {
    void *payload;
    uint32_t length;
    uint32_t seconds;
    uint32_t microseconds;
} sniffer_packet_info_t;

static sniffer_runtime_t snf_rt = {0};
static wlan_filter_table_t wifi_filter_hash_table[SNIFFER_WLAN_FILTER_MAX] = {0};
static esp_err_t sniffer_stop(sniffer_runtime_t *sniffer);

static uint32_t hash_func(const char *str, uint32_t max_num)
{
    uint32_t ret = 0;
    char *p = (char *)str;
    while (*p) {
        ret += *p;
        p++;
    }
    return ret % max_num;
}

static void create_wifi_filter_hashtable(void)
{
    char *wifi_filter_keys[SNIFFER_WLAN_FILTER_MAX] = {"mgmt", "data", "ctrl", "misc", "mpdu", "ampdu", "fcsfail"};
    uint32_t wifi_filter_values[SNIFFER_WLAN_FILTER_MAX] = {WIFI_PROMIS_FILTER_MASK_MGMT, WIFI_PROMIS_FILTER_MASK_DATA,
                                                            WIFI_PROMIS_FILTER_MASK_CTRL, WIFI_PROMIS_FILTER_MASK_MISC,
                                                            WIFI_PROMIS_FILTER_MASK_DATA_MPDU, WIFI_PROMIS_FILTER_MASK_DATA_AMPDU,
                                                            WIFI_PROMIS_FILTER_MASK_FCSFAIL
                                                           };
    for (int i = 0; i < SNIFFER_WLAN_FILTER_MAX; i++) {
        uint32_t idx = hash_func(wifi_filter_keys[i], SNIFFER_WLAN_FILTER_MAX);
        while (wifi_filter_hash_table[idx].filter_name) {
            idx++;
            if (idx >= SNIFFER_WLAN_FILTER_MAX) {
                idx = 0;
            }
        }
        wifi_filter_hash_table[idx].filter_name = wifi_filter_keys[i];
        wifi_filter_hash_table[idx].filter_val = wifi_filter_values[i];
    }
}

static uint32_t search_wifi_filter_hashtable(const char *key)
{
    uint32_t len = strlen(key);
    uint32_t start_idx = hash_func(key, SNIFFER_WLAN_FILTER_MAX);
    uint32_t idx = start_idx;
    while (strncmp(wifi_filter_hash_table[idx].filter_name, key, len)) {
        idx++;
        if (idx >= SNIFFER_WLAN_FILTER_MAX) {
            idx = 0;
        }
        /* wrong key */
        if (idx == start_idx) {
            return 0;
        }
    }
    return wifi_filter_hash_table[idx].filter_val;
}

static void queue_packet(void *recv_packet, sniffer_packet_info_t *packet_info)
{
    /* Copy a packet from Link Layer driver and queue the copy to be processed by sniffer task */
    void *packet_to_queue = malloc(packet_info->length);
    if (packet_to_queue) {
        memcpy(packet_to_queue, recv_packet, packet_info->length);
        packet_info->payload = packet_to_queue;
        if (snf_rt.work_queue) {
            /* send packet_info */
            if (xQueueSend(snf_rt.work_queue, packet_info, pdMS_TO_TICKS(SNIFFER_PROCESS_PACKET_TIMEOUT_MS)) != pdTRUE) {
                ESP_LOGE(SNIFFER_TAG, "sniffer work queue full");
                free(packet_info->payload);
            }
        }
    } else {
        ESP_LOGE(SNIFFER_TAG, "No enough memory for promiscuous packet");
    }
}

static void wifi_sniffer_cb(void *recv_buf, wifi_promiscuous_pkt_type_t type)
{
    sniffer_packet_info_t packet_info;
    wifi_promiscuous_pkt_t *sniffer = (wifi_promiscuous_pkt_t *)recv_buf;
    /* prepare packet_info */
    packet_info.seconds = sniffer->rx_ctrl.timestamp / 1000000U;
    packet_info.microseconds = sniffer->rx_ctrl.timestamp % 1000000U;
    packet_info.length = sniffer->rx_ctrl.sig_len;

    /* For now, the sniffer only dumps the length of the MISC type frame */
    if (type != WIFI_PKT_MISC && !sniffer->rx_ctrl.rx_state) {
        packet_info.length -= SNIFFER_PAYLOAD_FCS_LEN;
        queue_packet(sniffer->payload, &packet_info);
    }
}

static void sniffer_task(void *parameters)
{
    sniffer_packet_info_t packet_info;
    sniffer_runtime_t *sniffer = (sniffer_runtime_t *)parameters;

    while (sniffer->is_running) {
        if (sniffer->packets_to_sniff == 0) {
            sniffer_stop(sniffer);
            break;
        }
        /* receive packet info from queue */
        if (xQueueReceive(sniffer->work_queue, &packet_info, pdMS_TO_TICKS(SNIFFER_PROCESS_PACKET_TIMEOUT_MS)) != pdTRUE) {
            continue;
        }
        if (packet_capture(packet_info.payload, packet_info.length, packet_info.seconds,
                           packet_info.microseconds) != ESP_OK) {
            ESP_LOGW(SNIFFER_TAG, "save captured packet failed");
        }
        free(packet_info.payload);
        if (sniffer->packets_to_sniff > 0) {
            sniffer->packets_to_sniff--;
        }

    }
    /* notify that sniffer task is over */
    if (sniffer->packets_to_sniff != 0) {
        xSemaphoreGive(sniffer->sem_task_over);
    }
    vTaskDelete(NULL);
}

static esp_err_t sniffer_stop(sniffer_runtime_t *sniffer)
{
    bool eth_set_promiscuous;
    esp_err_t ret = ESP_OK;

    if (!sniffer->is_running) {
		ESP_LOGE(SNIFFER_TAG, "sniffer is already stopped");
		return -1;
	}

    if (esp_wifi_set_promiscuous(false) != ESP_OK) {
		ESP_LOGE(SNIFFER_TAG, "stop wifi promiscuous failed");
		return -1;
	}
    ESP_LOGI(SNIFFER_TAG, "stop promiscuous ok");

    /* stop sniffer local task */
    sniffer->is_running = false;
    /* wait for task over */
    if (sniffer->packets_to_sniff != 0) {
        xSemaphoreTake(sniffer->sem_task_over, portMAX_DELAY);
    }

    vSemaphoreDelete(sniffer->sem_task_over);
    sniffer->sem_task_over = NULL;
    /* make sure to free all resources in the left items */
    UBaseType_t left_items = uxQueueMessagesWaiting(sniffer->work_queue);

    sniffer_packet_info_t packet_info;
    while (left_items--) {
        xQueueReceive(sniffer->work_queue, &packet_info, pdMS_TO_TICKS(SNIFFER_PROCESS_PACKET_TIMEOUT_MS));
        free(packet_info.payload);
    }
    vQueueDelete(sniffer->work_queue);
    sniffer->work_queue = NULL;

    /* stop pcap session */
    sniff_packet_stop();
    return ret;
}

static esp_err_t sniffer_start(sniffer_runtime_t *sniffer)
{
    esp_err_t ret = ESP_OK;
    pcap_link_type_t link_type;
    wifi_promiscuous_filter_t wifi_filter;
    bool eth_set_promiscuous;

    if (!(sniffer->is_running)) {
		ESP_LOGE(SNIFFER_TAG, "sniffer is already running");
		return -1;
	}

	link_type = PCAP_LINK_TYPE_802_11;
    /* init a pcap session */
    sniff_packet_start(link_type);

    sniffer->is_running = true;
    sniffer->work_queue = xQueueCreate(SNIFFER_WORK_QUEUE_LEN, sizeof(sniffer_packet_info_t));
    if (!sniffer->work_queue) {
		goto err_queue;
	}
	sniffer->sem_task_over = xSemaphoreCreateBinary();
	if (!sniffer->sem_task_over) {
		goto err_sem;
	}
    
	if (xTaskCreate(sniffer_task, "snifferT", SNIFFER_TASK_STACK_SIZE,
		sniffer, SNIFFER_TASK_PRIORITY, &sniffer->task) != ESP_OK) {
		goto err_task;
	} 		

	wifi_filter.filter_mask = sniffer->filter;
	esp_wifi_set_promiscuous_filter(&wifi_filter);
	esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_cb);
	if (esp_wifi_set_promiscuous(true) != ESP_OK) {
		goto err_start;
	}
	esp_wifi_set_channel(sniffer->channel, WIFI_SECOND_CHAN_NONE);
	ESP_LOGI(SNIFFER_TAG, "start WiFi promiscuous ok");
    return ret;

err_start:
    vTaskDelete(sniffer->task);
    sniffer->task = NULL;
err_task:
    vSemaphoreDelete(sniffer->sem_task_over);
    sniffer->sem_task_over = NULL;
err_sem:
    vQueueDelete(sniffer->work_queue);
    sniffer->work_queue = NULL;
err_queue:
    sniffer->is_running = false;
    return ret;
}

int do_sniffer_cmd(sniffer_args_t* args_sniff )
{
    /* Check whether or not to stop sniffer: "--stop" option */
    if (args_sniff->stop) {
        /* stop sniffer */
        sniffer_stop(&snf_rt);
        return 0;
    }

    /* Check interface: "-i" option */

	snf_rt.interf = SNIFFER_INTF_WLAN;
    snf_rt.channel = args_sniff->channel;
    
	/* Check filter setting: "-F" option */
    if (!args_sniff->filters.empty()) {
		snf_rt.filter = 0;
		for (int i = 0; i < args_sniff->filters.size(); i++) {
			snf_rt.filter += search_wifi_filter_hashtable(args_sniff->filters[i].c_str());
		}
		/* When filter conditions are all wrong */
		if (snf_rt.filter == 0) {
			snf_rt.filter = WIFI_PROMIS_FILTER_MASK_ALL;
		}
	} else {
		snf_rt.filter = WIFI_PROMIS_FILTER_MASK_ALL;
	}

    /* Check the number of captured packages: "-n" option */
    snf_rt.packets_to_sniff = -1;
    if (args_sniff->number > 0) {
        snf_rt.packets_to_sniff = args_sniff->number; 
        ESP_LOGI(SNIFFER_TAG, "%d packages will be captured", snf_rt.packets_to_sniff);
    }

    /* start sniffer */
    sniffer_start(&snf_rt);
    return 0;
}

// static void sniffer_task(void *parameters)
// {
//     sniffer_packet_info_t packet_info;
//     sniffer_runtime_t *sniffer = (sniffer_runtime_t *)parameters;

//     while (sniffer->is_running) {
//         if (sniffer->packets_to_sniff == 0) {
//             sniffer_stop(sniffer);
//             break;
//         }
//         /* receive packet info from queue */
//         if (xQueueReceive(sniffer->work_queue, &packet_info, pdMS_TO_TICKS(SNIFFER_PROCESS_PACKET_TIMEOUT_MS)) != pdTRUE) {
//             continue;
//         }
//         if (packet_capture(packet_info.payload, packet_info.length, packet_info.seconds,
//                            packet_info.microseconds) != ESP_OK) {
//             ESP_LOGW(SNIFFER_TAG, "save captured packet failed");
//         }
//         free(packet_info.payload);
//         if (sniffer->packets_to_sniff > 0) {
//             sniffer->packets_to_sniff--;
//         }

//     }
//     /* notify that sniffer task is over */
//     if (sniffer->packets_to_sniff != 0) {
//         xSemaphoreGive(sniffer->sem_task_over);
//     }
//     vTaskDelete(NULL);
// }