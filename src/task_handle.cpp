#include "task_handle.h"
#include "sniffer.h"
#include "sd_adapter.h"
#include "netw_scan.h"

#include "esp_log.h"

const char *Task::TAG = "Task";
FILE *f_to_write;

Task::Task() : duration(1), id(0), omode(outputMode::JSON_RESPONSE), tt(task_type::SLEEP), params(nullptr) {
    this->filename = "/task_id" + std::to_string(this->id);    
}
Task::Task(int32_t mid, outputMode om, int32_t dur, task_type mtt) : duration(dur), id(mid),
        omode(om), tt(mtt), params(nullptr) {
    this->filename = "/task_id" + std::to_string(this->id); 
    switch (tt) {
        case task_type::SCAN_STA:
        case task_type::SNIFF_CHANNEL:
        case task_type::SNIFF_STA:
            params = calloc(1, sizeof(sniffer_args_t));
            if (tt == task_type::SCAN_STA)
                ((sniffer_args_t*)params)->is_sta_sniff_mode = 1;
            break;
        case task_type::SCAN_AP_ALL:
        case task_type::SCAN_AP_PARAMS:
            params = calloc(1, sizeof(wifi_scan_config_t));
            break;
        case task_type::SLEEP:
            break;
        default:
            break;
    }
}
Task::Task(Task &&task) : duration(task.duration), id(task.id),
        omode(task.omode), tt(task.tt), filename(std::move(task.filename)), params(task.params) {
    task.params = nullptr;
}

Task::Task(const Task &task) : duration(task.duration), id(task.id),
        omode(task.omode), tt(task.tt), filename(task.filename) {  
    switch (tt)
    {
    case task_type::SCAN_STA:
    case task_type::SNIFF_CHANNEL: {
        params = calloc(1, sizeof(sniffer_args_t));
        ((sniffer_args_t *)params)->number = ((const sniffer_args_t *)task.params)->number;
        ((sniffer_args_t *)params)->channel = ((const sniffer_args_t *)task.params)->channel;
        ((sniffer_args_t *)params)->is_sta_sniff_mode = ((const sniffer_args_t *)task.params)->is_sta_sniff_mode;
        if (tt == task_type::SNIFF_CHANNEL)  
            ((sniffer_args_t *)params)->filters = ((const sniffer_args_t *)task.params)->filters;
        break;
    }
    case task_type::SNIFF_STA:  {
        params = calloc(1, sizeof(sniffer_args_t));
        ((sniffer_args_t *)params)->channel = ((const sniffer_args_t *)task.params)->channel;
        memcpy(((sniffer_args_t *)params)->bssid, ((const sniffer_args_t *)task.params)->bssid, 8*sizeof(uint8_t));
        ((sniffer_args_t *)params)->filters = ((const sniffer_args_t *)task.params)->filters;
        break; 
    } 
    case task_type::SCAN_AP_ALL: {
        params = calloc(1, sizeof(wifi_scan_config_t));
        ((wifi_scan_config_t *)params)->channel = ((const wifi_scan_config_t *)task.params)->channel;
        ((wifi_scan_config_t *)params)->show_hidden = ((const wifi_scan_config_t *)task.params)->show_hidden;
        break; 
    } 
    case task_type::SCAN_AP_PARAMS: {        
        params = calloc(1, sizeof(wifi_scan_config_t));
        ((wifi_scan_config_t *)params)->channel = ((const wifi_scan_config_t *)task.params)->channel;
        ((wifi_scan_config_t *)params)->show_hidden = ((const wifi_scan_config_t *)task.params)->show_hidden;
        break; 
    }
    case task_type::SLEEP:
        break;
    default:
        break;
    }
}

Task::~Task() {
    if(params) {
        switch (tt)
        {

            case task_type::SCAN_AP_ALL:
            case task_type::SCAN_AP_PARAMS:
                if (((wifi_scan_config_t *)params)->bssid)
                    free(((wifi_scan_config_t *)params)->bssid);
                if (((wifi_scan_config_t *)params)->ssid)
                    free(((wifi_scan_config_t *)params)->bssid);
            break;
            case task_type::SCAN_STA:
            case task_type::SNIFF_CHANNEL:
            case task_type::SNIFF_STA:                
            break;
        default:
            break;
        }
        free(params);
    }
}

void Task::parseJson_parameters(const JsonObject &ref) {
    if (ref.containsKey("filename")) {
        this->filename = ref["filename"].as<const char*>();
    }
    else {
        this->filename = "/task_id" + std::to_string(this->id);
    }

    switch (tt)
    {
    case task_type::SCAN_STA: {
        ((sniffer_args_t *)params)->is_sta_sniff_mode = 1;
        ((sniffer_args_t *)params)->number = ref["count"]| -1;
        ((sniffer_args_t *)params)->channel = ref["channel"] | 1;
        ((sniffer_args_t *)params)->stop = 0;
        break;
    }
    case task_type::SNIFF_STA: {
        if (ref.containsKey("bssid")) {
            const char *val = ref["bssid"].as<const char*>();
            for(int i = 0; i < 6; i++) {
               ((sniffer_args_t *)params)->bssid[i] = strtol(val, NULL, 16);
               val += 3;
            }
        }
    }
    case task_type::SNIFF_CHANNEL: {
        ((sniffer_args_t *)params)->number = ref["count"]| -1;
        ((sniffer_args_t *)params)->channel = ref["channel"] | 1;
        ((sniffer_args_t *)params)->stop = 0;
        if (ref.containsKey("filter") && ref["filter"].is<JsonArray>()) {
	        for (int i = 0; i < ref["filter"].as<JsonArray>().size(); i++) {
                ((sniffer_args_t *)params)->filters.push_back(ref["filter"][i].as<const char*>());
            }
        }
        break;
    }
    case task_type::SCAN_AP_ALL: {
         ((wifi_scan_config_t *)params)->show_hidden = ref["show_hidden"] | true;
        break;
    } 
    case task_type::SCAN_AP_PARAMS: {
        if (ref.containsKey("bssid")) {
            ((wifi_scan_config_t *)params)->bssid = (unsigned char*)malloc(sizeof(unsigned int) * 6);
            const char *val = ref["bssid"].as<const char*>();
            for(int i = 0; i < 6; i++) {
               ((wifi_scan_config_t *)params)->bssid[i] = strtol(val, NULL, 16);
               val += 3;
            }
        }
        if (ref.containsKey("ssid")) {
            ((wifi_scan_config_t *)params)->ssid = (unsigned char*)malloc(sizeof(unsigned int) * strlen(ref["ssid"].as<const char*>()));
            memcpy(((wifi_scan_config_t *)params)->ssid, (unsigned char*)(ref["bssid"].as<const char*>()), strlen(ref["ssid"].as<const char*>()));
        }
        ((wifi_scan_config_t *)params)->bssid = (unsigned char*)(ref["bssid"].as<const char*>());
        ((wifi_scan_config_t *)params)->ssid = (unsigned char*)(ref["channel"].as<const char*>());
        ((wifi_scan_config_t *)params)->channel = ref["channel"] | 1;
        ((wifi_scan_config_t *)params)->show_hidden = ref["show_hidden"] | true;
        break;
    }
    case task_type::SLEEP:
        break;
    default:
        break;
    }
}

void Task::do_task() {
    switch (tt)
    {
    case task_type::SNIFF_STA:
    case task_type::SNIFF_CHANNEL: {	
        pcap_args start_pcap;
        sniffer_args_t *start_sniff = (sniffer_args_t *)params;
        start_pcap.close = 0;
        start_pcap.file = filename.c_str();
        start_pcap.open = 1;
        start_pcap.summary = 0;

        do_pcap_cmd(&start_pcap);
        ESP_LOGI(TAG, LOG_COLOR(LOG_COLOR_BROWN) "do_pcap_cmd");
        do_sniffer_cmd(start_sniff);
        ESP_LOGI(TAG, LOG_COLOR(LOG_COLOR_BROWN) "do_sniffer_cmd sniff channel");
        break;
    }
    case task_type::SCAN_STA: {
        sniffer_args_t *start_sniff = (sniffer_args_t *)params;
        ESP_LOGI(TAG, "Open file for write: %s", std::string(SNIFFER_MOUNT_POINT + filename + ".txt").c_str());
        f_to_write = fopen(std::string(SNIFFER_MOUNT_POINT + filename + ".txt").c_str(), "w");
        do_sniffer_cmd(start_sniff);
        ESP_LOGI(TAG, LOG_COLOR(LOG_COLOR_BROWN) "do_sniffer_cmd scan STA");
        break;
    }
    case task_type::SCAN_AP_ALL: {
        wifi_netw_scan(this->filename);
        break;
    }
    case task_type::SCAN_AP_PARAMS: {
        wifi_scan_config_t *start_scan = (wifi_scan_config_t *)params;
        wifi_netw_scan_with_config(start_scan, this->filename);
        break;
    }
    case task_type::SLEEP:
        break;
    default:
        break;
    }
}

void Task::set_task_type(task_type mtt) {
    tt = mtt;
    if (params)
        free(params);
    
    switch (tt) {
    case task_type::SCAN_STA:
    case task_type::SNIFF_CHANNEL:
    case task_type::SNIFF_STA:
        params = calloc(1, sizeof(sniffer_args_t));
        break;
    case task_type::SCAN_AP_ALL:
    case task_type::SCAN_AP_PARAMS:
        params = calloc(1, sizeof(wifi_scan_config_t));
        break;
    case task_type::SLEEP:
        break;
    default:
        break;
    }
}

void Task::stop_task() {
    switch (tt)
    {
    case task_type::SNIFF_STA:
    case task_type::SNIFF_CHANNEL: {        
        pcap_args stop_pcap;
        stop_pcap.close = 1;
        sniffer_args_t stop_sniff;
        stop_sniff.stop = 1;
        do_sniffer_cmd(&stop_sniff);
        do_pcap_cmd(&stop_pcap);
        break;
    }
    case task_type::SCAN_STA : {       
        sniffer_args_t stop_sniff;
        stop_sniff.stop = 1;
        do_sniffer_cmd(&stop_sniff);
        break;
    }
    default:
        break;
    }
}