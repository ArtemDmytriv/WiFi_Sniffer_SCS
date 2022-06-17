/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "sd_adapter.h"

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "pcap";

#define PCAP_MAGIC_BIG_ENDIAN 0xA1B2C3D4    /*!< Big-Endian */
#define PCAP_MAGIC_LITTLE_ENDIAN 0xD4C3B2A1 /*!< Little-Endian */

pcap_cmd_runtime_t pcap_cmd_rt = {0};

typedef struct pcap_file_t pcap_file_t;

/**
 * @brief Pcap File Header
 *
 */
typedef struct {
    uint32_t magic;     /*!< Magic Number */
    uint16_t major;     /*!< Major Version */
    uint16_t minor;     /*!< Minor Version */
    uint32_t zone;      /*!< Time Zone Offset */
    uint32_t sigfigs;   /*!< Timestamp Accuracy */
    uint32_t snaplen;   /*!< Max Length to Capture */
    uint32_t link_type; /*!< Link Layer Type */
} pcap_file_header_t;

/**
 * @brief Pcap Packet Header
 *
 */
typedef struct {
    uint32_t seconds;        /*!< Number of seconds since January 1st, 1970, 00:00:00 GMT */
    uint32_t microseconds;   /*!< Number of microseconds when the packet was captured (offset from seconds) */
    uint32_t capture_length; /*!< Number of bytes of captured data, no longer than packet_length */
    uint32_t packet_length;  /*!< Actual length of current packet */
} pcap_packet_header_t;

/**
 * @brief Pcap Runtime Handle
 *
 */
struct pcap_file_t {
    FILE *file;                 /*!< File handle */
    pcap_link_type_t link_type; /*!< Pcap Link Type */
    unsigned int major_version; /*!< Pcap version: major */
    unsigned int minor_version; /*!< Pcap version: minor */
    unsigned int time_zone;     /*!< Pcap timezone code */
    uint32_t endian_magic;      /*!< Magic value related to endian format */
};

static esp_err_t pcap_close(pcap_cmd_runtime_t *pcap)
{
    esp_err_t ret = ESP_OK;
    if(!pcap->is_opened) {
        ESP_LOGE(TAG, ".pcap file is already closed");
        goto err;
    }

    if (pcap_del_session(pcap->pcap_handle) != ESP_OK) {
        ESP_LOGE(TAG, "stop pcap session failed");
        goto err;
    }
    pcap->is_opened = false;
    pcap->link_type_set = false;
    pcap->pcap_handle = NULL;
err:
    return ret;
}

static esp_err_t pcap_open(pcap_cmd_runtime_t *pcap)
{
    esp_err_t ret = ESP_OK;
    /* Create file to write, binary format */

    FILE *fp = NULL;
    ESP_LOGI(TAG, "Opening file %s", pcap->filename);
    fp = fopen(pcap->filename, "wb+");

    if (!fp) {
        ESP_LOGE(TAG, "open file failed");
        goto err;
    }
    pcap_config_t pcap_config;
    pcap_config.fp = fp;
    pcap_config.major_version = PCAP_DEFAULT_VERSION_MAJOR;
    pcap_config.minor_version = PCAP_DEFAULT_VERSION_MINOR;
    pcap_config.time_zone = PCAP_DEFAULT_TIME_ZONE_GMT;
    if (pcap_new_session(&pcap_config, &pcap_cmd_rt.pcap_handle) != ESP_OK) {
        ESP_LOGE(TAG, "pcap init failed");
        goto err;
    }
    
    pcap->is_opened = true;
    
    ESP_LOGI(TAG, "open file successfully");
    return ret;
err:
    if (fp) {
        fclose(fp);
    }
    return ret;
}

int32_t pcap_new_session(const pcap_config_t *config, pcap_file_handle_t *ret_pcap)
{
    int32_t ret = 0;
    pcap_file_t *pcap = NULL;
    pcap = (pcap_file_t *)calloc(1, sizeof(pcap_file_t));
    if (!pcap) {
        ESP_LOGE(TAG, "no mem for pcap file object");
        goto err;
    }   

    pcap->file = config->fp;
    pcap->major_version = config->major_version;
    pcap->minor_version = config->minor_version;
    pcap->endian_magic = config->flags.little_endian ? PCAP_MAGIC_LITTLE_ENDIAN : PCAP_MAGIC_BIG_ENDIAN;
    pcap->time_zone = config->time_zone;
    *ret_pcap = pcap;
    return ret;
err:
    if (pcap) {
        free(pcap);
    }
    return ret;
}

esp_err_t packet_capture(void *payload, uint32_t length, uint32_t seconds, uint32_t microseconds)
{
    return pcap_capture_packet(pcap_cmd_rt.pcap_handle, payload, length, seconds, microseconds);
}

//??

esp_err_t sniff_packet_start(pcap_link_type_t link_type)
{
    esp_err_t ret = ESP_OK;

    if (!pcap_cmd_rt.is_opened) {
        ESP_LOGE(TAG,"no .pcap file stream is open");
        goto err;
    }

    if (pcap_cmd_rt.link_type_set) {
        if (link_type != pcap_cmd_rt.link_type) {
            ESP_LOGE(TAG,"link type error");
            goto err;
        }
        if (pcap_cmd_rt.is_writing) {
            ESP_LOGE(TAG,"still sniffing");
            goto err;
        }
    } 
    else {
        pcap_cmd_rt.link_type = link_type;
        /* Create file to write, binary format */
        pcap_write_header(pcap_cmd_rt.pcap_handle, link_type);
        pcap_cmd_rt.link_type_set = true;
    }
    pcap_cmd_rt.is_writing = true;
err:
    return ret;
}

esp_err_t sniff_packet_stop(void)
{
    pcap_cmd_rt.is_writing = false;
    return ESP_OK;
}

int do_pcap_cmd(pcap_args *pcapa)
{
    int ret = 0;

    /* Check whether or not to close pcap file: "--close" option */
    if (pcapa->close) {
        /* close the pcap file */
        if (pcap_cmd_rt.is_writing) {
            ESP_LOGE(TAG, "still sniffing, file will not close");
            return ESP_FAIL;
        }
        pcap_close(&pcap_cmd_rt);
        ESP_LOGI(TAG, ".pcap file close done");
        return ESP_FAIL;
    }

    //??
    /* set pcap file name: "-f" option */
    int len = snprintf(pcap_cmd_rt.filename, sizeof(pcap_cmd_rt.filename), "%s%s.pcap", SNIFFER_MOUNT_POINT, pcapa->file.c_str());
    if (len >= sizeof(pcap_cmd_rt.filename)) {
        ESP_LOGW(TAG, "pcap file name too long, try to enlarge memory in menuconfig");
    }

    /* Check if needs to be parsed and shown: "--summary" option */
    if (pcapa->summary) {
        ESP_LOGI(TAG, "%s is to be parsed", pcap_cmd_rt.filename);
        if (pcap_cmd_rt.is_opened) {
            if (pcap_cmd_rt.is_writing) {
                ESP_LOGE(TAG, "still writing");
                return ESP_FAIL;
            }            
            if (pcap_print_summary(pcap_cmd_rt.pcap_handle, stdout) != ESP_OK) {
                ESP_LOGE(TAG, "pcap print summary failed");
                return ESP_FAIL;
            }
        } else {
            FILE *fp;
            fp = fopen(pcap_cmd_rt.filename, "rb");
            if(!fp) {
                ESP_LOGE(TAG, "open file failed");
                return ESP_FAIL;
            }

            pcap_config_t pcap_config;
            pcap_config.fp = fp;
            pcap_config.major_version = PCAP_DEFAULT_VERSION_MAJOR;
            pcap_config.minor_version = PCAP_DEFAULT_VERSION_MINOR;
            pcap_config.time_zone = PCAP_DEFAULT_TIME_ZONE_GMT;

            if (pcap_new_session(&pcap_config, &pcap_cmd_rt.pcap_handle) != ESP_OK) {
                ESP_LOGE(TAG, "open file failed");
                return ESP_FAIL;
            }
            if (pcap_print_summary(pcap_cmd_rt.pcap_handle, stdout) != ESP_OK) {
                ESP_LOGE(TAG, "pcap print summary failed");
                return ESP_FAIL;
            }
            if (pcap_del_session(pcap_cmd_rt.pcap_handle) != ESP_OK) {
                ESP_LOGE(TAG, "stop pcap session failed");
                return ESP_FAIL;
            }
        }
    }

    if (pcapa->open) {
        pcap_open(&pcap_cmd_rt);
    }
    return ret;
}

//??

int32_t pcap_del_session(pcap_file_handle_t pcap)
{
    if (pcap->file) {
        fclose(pcap->file);
        pcap->file = NULL;
    }
    free(pcap);
    return 0;
}

int32_t pcap_write_header(pcap_file_handle_t pcap, pcap_link_type_t link_type)
{
    /* Write Pcap File header */
    pcap_file_header_t header;
    header.magic = pcap->endian_magic;
    header.major = static_cast<uint16_t>(pcap->major_version);
    header.minor = static_cast<uint16_t>(pcap->minor_version);
    header.zone = pcap->time_zone;
    header.sigfigs = 0;
    header.snaplen = 0x40000;
    header.link_type = link_type;

    size_t real_write = fwrite(&header, sizeof(header), 1, pcap->file);
    if (real_write != 1) {
        ESP_LOGE(TAG, "write pcap file header failed");
        return -1;
    }
    /* Save the link type to pcap file object */
    pcap->link_type = link_type;
    /* Flush content in the buffer into device */
    fflush(pcap->file);
    return 0;
}

int32_t pcap_capture_packet(pcap_file_handle_t pcap, void *payload, uint32_t length, uint32_t seconds, uint32_t microseconds)
{
    size_t real_write = 0;
    pcap_packet_header_t header = {
        .seconds = seconds,
        .microseconds = microseconds,
        .capture_length = length,
        .packet_length = length
    };

    real_write = fwrite(&header, sizeof(header), 1, pcap->file);
    if (real_write != 1) {
        ESP_LOGE(TAG, "write packet header failed");
        return -1;
    }
    real_write = fwrite(payload, sizeof(uint8_t), length, pcap->file);
    if (real_write != length) {
        ESP_LOGE(TAG, "write packet payload failed");
        return -1;
    }

    fflush(pcap->file);
    return 0;
}

int32_t pcap_print_summary(pcap_file_handle_t pcap, FILE *print_file)
{
    int32_t ret = 0;
    long size = 0;
    char *packet_payload = NULL;
    
    // get file size
    fseek(pcap->file, 0L, SEEK_END);
    size = ftell(pcap->file);
    fseek(pcap->file, 0L, SEEK_SET);
    // file empty is allowed, so return 0
    if (size == 0) {
        return 0;
    }
    // packet index (by bytes)
    uint32_t index = 0;
    pcap_file_header_t file_header;
    size_t real_read = fread(&file_header, sizeof(pcap_file_header_t), 1, pcap->file);
    
    if (real_read != 1) {
        return -1;
    }
    index += sizeof(pcap_file_header_t);
    //print pcap header information
    fprintf(print_file, "------------------------------------------------------------------------\n");
    fprintf(print_file, "Pcap packet Head:\n");
    fprintf(print_file, "------------------------------------------------------------------------\n");
    fprintf(print_file, "Magic Number: %x\n", file_header.magic);
    fprintf(print_file, "Major Version: %d\n", file_header.major);
    fprintf(print_file, "Minor Version: %d\n", file_header.minor);
    fprintf(print_file, "SnapLen: %d\n", file_header.snaplen);
    fprintf(print_file, "LinkType: %d\n", file_header.link_type);
    fprintf(print_file, "------------------------------------------------------------------------\n");
    uint32_t packet_num = 0;
    pcap_packet_header_t packet_header;
    while (index < size) {
        real_read = fread(&packet_header, sizeof(pcap_packet_header_t), 1, pcap->file);

        if (real_read != 1) {
            ESP_LOGE(TAG, "read pcap packet header failed");
            return -1;
        }
        // print packet header information
        fprintf(print_file, "Packet %d:\n", packet_num);
        fprintf(print_file, "Timestamp (Seconds): %d\n", packet_header.seconds);
        fprintf(print_file, "Timestamp (Microseconds): %d\n", packet_header.microseconds);
        fprintf(print_file, "Capture Length: %d\n", packet_header.capture_length);
        fprintf(print_file, "Packet Length: %d\n", packet_header.packet_length);
        size_t payload_length = packet_header.capture_length;
        packet_payload = (char*) malloc(payload_length);
        
        if (!packet_payload) {
            ESP_LOGE(TAG,"no mem to save packet payload");
            goto err;
        }
        real_read = fread(packet_payload, payload_length, 1, pcap->file);
        if (real_read != 1) {
            ESP_LOGE(TAG, "read payload error");
            goto err;
        }
        // print packet information
        if (file_header.link_type == PCAP_LINK_TYPE_802_11) {
            // Frame Control Field is coded as LSB first
            fprintf(print_file, "Frame Type: %2x\n", (packet_payload[0] >> 2) & 0x03);
            fprintf(print_file, "Frame Subtype: %2x\n", (packet_payload[0] >> 4) & 0x0F);
            fprintf(print_file, "Destination: ");
            for (int j = 0; j < 5; j++) {
                fprintf(print_file, "%2x ", packet_payload[4 + j]);
            }
            fprintf(print_file, "%2x\n", packet_payload[9]);
            fprintf(print_file, "Source: ");
            for (int j = 0; j < 5; j++) {
                fprintf(print_file, "%2x ", packet_payload[10 + j]);
            }
            fprintf(print_file, "%2x\n", packet_payload[15]);
            fprintf(print_file, "------------------------------------------------------------------------\n");
        } else if (file_header.link_type == PCAP_LINK_TYPE_ETHERNET) {
            fprintf(print_file, "Destination: ");
            for (int j = 0; j < 5; j++) {
                fprintf(print_file, "%2x ", packet_payload[j]);
            }
            fprintf(print_file, "%2x\n", packet_payload[5]);
            fprintf(print_file, "Source: ");
            for (int j = 0; j < 5; j++) {
                fprintf(print_file, "%2x ", packet_payload[6 + j]);
            }
            fprintf(print_file, "%2x\n", packet_payload[11]);
            fprintf(print_file, "Type: 0x%x\n", packet_payload[13] | (packet_payload[12] << 8));
            fprintf(print_file, "------------------------------------------------------------------------\n");
        } else {
            fprintf(print_file, "Unknown link type:%d\n", file_header.link_type);
            fprintf(print_file, "------------------------------------------------------------------------\n");
        }
        free(packet_payload);
        packet_payload = NULL;
        index += packet_header.capture_length + sizeof(pcap_packet_header_t);
        packet_num ++;
    }
    fprintf(print_file, "Pcap packet Number: %d\n", packet_num);
    fprintf(print_file, "------------------------------------------------------------------------\n");
    return ret;
err:
    if (packet_payload) {
        free(packet_payload);
    }
    return ret;
}

/** 'mount' command */
int mount_sd()
{
    int32_t ret;
    ESP_LOGI(TAG, "Initializing SD card");

    // Set up SPI bus
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg;
    memset(&bus_cfg, 0, sizeof(spi_bus_config_t));
    bus_cfg.mosi_io_num = PIN_NUM_MOSI;
    bus_cfg.miso_io_num = PIN_NUM_MISO;
    bus_cfg.sclk_io_num = PIN_NUM_CLK;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = 4000;

    ret = spi_bus_initialize((spi_host_device_t)HSPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return 1;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = (spi_host_device_t)HSPI_HOST;

    // Fat FS configuration options
    esp_vfs_fat_mount_config_t mount_config;
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 4;

    // This is the information we'll be given in return
    sdmmc_card_t *sdcard_info;

    ret = esp_vfs_fat_sdspi_mount(SNIFFER_MOUNT_POINT, &host, &slot_config, &mount_config, &sdcard_info);

    if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                ESP_LOGE(TAG, "Failed to mount filesystem. "
                         "If you want the card to be formatted, set format_if_mount_failed = true.");
            } else {
                ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                         "Make sure SD card lines have pull-up resistors in place.",
                         esp_err_to_name(ret));
            }
            return 1;
        }
    /* print card info if mount successfully */
    sdmmc_card_print_info(stdout, sdcard_info);

    return 0;
}

int unmount_sd()
{
    if (esp_vfs_fat_sdmmc_unmount() != ESP_OK) {
        ESP_LOGE(TAG, "Card unmount failed");
        return -1;
    }
    ESP_LOGI(TAG, "Card unmounted");
    return 0;
}
