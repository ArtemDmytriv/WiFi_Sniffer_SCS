#ifndef _SD_ADAPTER
#define _SD_ADAPTER

#include "stdio.h"
#include "esp_err.h"

#define FILE_NAME_MAX_LEN 64 
#define SNIFFER_MOUNT_POINT "/sdcard"
#define PIN_NUM_MISO GPIO_NUM_19
#define PIN_NUM_MOSI GPIO_NUM_23
#define PIN_NUM_CLK  GPIO_NUM_18
#define PIN_NUM_CS   GPIO_NUM_5

#define PCAP_DEFAULT_VERSION_MAJOR 0x02 /*!< Major Version */
#define PCAP_DEFAULT_VERSION_MINOR 0x04 /*!< Minor Version */
#define PCAP_DEFAULT_TIME_ZONE_GMT 0x00 /*!< Time Zone */

/**
 * @brief Type of pcap file handle
 *
 */
typedef struct pcap_file_t *pcap_file_handle_t;

/**
* @brief Link layer Type Definition, used for Pcap reader to decode payload
*
*/
typedef enum {
    PCAP_LINK_TYPE_LOOPBACK = 0,       /*!< Loopback devices, except for later OpenBSD */
    PCAP_LINK_TYPE_ETHERNET = 1,       /*!< Ethernet, and Linux loopback devices */
    PCAP_LINK_TYPE_TOKEN_RING = 6,     /*!< 802.5 Token Ring */
    PCAP_LINK_TYPE_ARCNET = 7,         /*!< ARCnet */
    PCAP_LINK_TYPE_SLIP = 8,           /*!< SLIP */
    PCAP_LINK_TYPE_PPP = 9,            /*!< PPP */
    PCAP_LINK_TYPE_FDDI = 10,          /*!< FDDI */
    PCAP_LINK_TYPE_ATM = 100,          /*!< LLC/SNAP encapsulated ATM */
    PCAP_LINK_TYPE_RAW_IP = 101,       /*!< Raw IP, without link */
    PCAP_LINK_TYPE_BSD_SLIP = 102,     /*!< BSD/OS SLIP */
    PCAP_LINK_TYPE_BSD_PPP = 103,      /*!< BSD/OS PPP */
    PCAP_LINK_TYPE_CISCO_HDLC = 104,   /*!< Cisco HDLC */
    PCAP_LINK_TYPE_802_11 = 105,       /*!< 802.11 */
    PCAP_LINK_TYPE_BSD_LOOPBACK = 108, /*!< OpenBSD loopback devices(with AF_value in network byte order) */
    PCAP_LINK_TYPE_LOCAL_TALK = 114    /*!< LocalTalk */
} pcap_link_type_t;

/**
* @brief Pcap configuration Type Definition
*
*/
typedef struct {
    FILE *fp;                   /*!< Pointer to a standard file handle */
    unsigned int major_version; /*!< Pcap version: major */
    unsigned int minor_version; /*!< Pcap version: minor */
    unsigned int time_zone;     /*!< Pcap timezone code */
    struct {
        unsigned int little_endian: 1; /*!< Whether the pcap file is recored in little endian format */
    } flags;
} pcap_config_t;

typedef struct {
    bool is_opened;
    bool is_writing;
    bool link_type_set;
    char filename[FILE_NAME_MAX_LEN];
    pcap_file_handle_t pcap_handle;
    pcap_link_type_t link_type;
} pcap_cmd_runtime_t;

typedef struct {
    const char *file;
    int open;
    int close;
    int summary;
} pcap_args;


extern pcap_cmd_runtime_t pcap_cmd_rt;

int32_t pcap_new_session(const pcap_config_t *config, pcap_file_handle_t *ret_pcap);
int32_t pcap_del_session(pcap_file_handle_t pcap);
int32_t pcap_write_header(pcap_file_handle_t pcap, pcap_link_type_t link_type);
int32_t pcap_capture_packet(pcap_file_handle_t pcap, void *payload, uint32_t length, uint32_t seconds, uint32_t microseconds);
int32_t pcap_print_summary(pcap_file_handle_t pcap, FILE *print_file);

int do_pcap_cmd(pcap_args *pcapa);
esp_err_t packet_capture(void *payload, uint32_t length, uint32_t seconds, uint32_t microseconds);
esp_err_t sniff_packet_start(pcap_link_type_t link_type);

int mount();
int unmount();

#endif // _SD_ADAPTER