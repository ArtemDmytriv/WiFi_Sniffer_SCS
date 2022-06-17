#include "task_handle.h"
#include "sniffer.h"
#include "sd_adapter.h"
#include "netw_scan.h"

#include "esp_log.h"

const char *Task::TAG = "Task";

Task::Task() : duration(1), id(0), omode(outputMode::JSON_RESPONSE), tt(task_type::SLEEP), params(nullptr) {
    this->filename = "/task_id" + std::to_string(this->id);    
}
Task::Task(int32_t mid, outputMode om, int32_t dur, task_type mtt) : duration(dur), id(mid),
        omode(om), tt(mtt), params(nullptr) {
    this->filename = "/task_id" + std::to_string(this->id); 
    switch (tt) {
        case task_type::SCAN_STA:
        case task_type::SNIFF_CHANNEL:
            params = calloc(1, sizeof(sniffer_args_t));
            if (tt == task_type::SCAN_STA)
                ((sniffer_args_t*)params)->is_sta_sniff_mode = 1;
            break;
        case task_type::SCAN_AP_ALL:
            params = calloc(1, sizeof(wifi_scan_config_t));
            break;
        case task_type::SCAN_AP_PARAMS:
        case task_type::SNIFF_STA:
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
        break;
    }
    case task_type::SCAN_AP_ALL: {
        params = calloc(1, sizeof(wifi_scan_config_t));
        ((wifi_scan_config_t *)params)->channel = ((const wifi_scan_config_t *)task.params)->channel;
        ((wifi_scan_config_t *)params)->show_hidden = ((const wifi_scan_config_t *)task.params)->show_hidden;
        break; 
    } 
    case task_type::SCAN_AP_PARAMS:
    case task_type::SNIFF_STA:
    case task_type::SLEEP:
        break;
    default:
        break;
    }
}

Task::~Task() {
    ESP_LOGI(TAG, "dtor");
    if(params) {
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
    case task_type::SCAN_STA:
    case task_type::SNIFF_CHANNEL: {
        ((sniffer_args_t *)params)->number = ref["count"].as<int>() | 1000;
        ((sniffer_args_t *)params)->channel = ref["channel"].as<int>() | 1;
        ((sniffer_args_t *)params)->stop = 0;
        break;
    }
    case task_type::SCAN_AP_ALL: {
        ((wifi_scan_config_t *)params)->channel = ref["channel"].as<uint8_t>() | (uint8_t)1;
        ((wifi_scan_config_t *)params)->show_hidden = ref["show_hidden"].as<bool>() | true;
        break;
    } 
    case task_type::SCAN_AP_PARAMS:
    case task_type::SNIFF_STA:
    case task_type::SLEEP:
        break;
    default:
        break;
    }
}

void Task::do_task() {
    switch (tt)
    {
    case task_type::SCAN_STA:
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
        ESP_LOGI(TAG, LOG_COLOR(LOG_COLOR_BROWN) "do_sniffer_cmd");
        break;
    }
    case task_type::SCAN_AP_ALL: {
        wifi_netw_scan(this->filename);
        break;
    }
    case task_type::SCAN_AP_PARAMS:
    case task_type::SNIFF_STA:
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
    case task_type::SNIFF_CHANNEL:
        params = calloc(1, sizeof(sniffer_args_t));
        break;
    case task_type::SCAN_STA:
    case task_type::SCAN_AP_ALL:
        params = calloc(1, sizeof(wifi_scan_config_t));
        break;
    case task_type::SCAN_AP_PARAMS:
    case task_type::SNIFF_STA:
    case task_type::SLEEP:
        break;
    default:
        break;
    }
}

void Task::stop_task() {
    switch (tt)
    {
    case task_type::SCAN_STA :
    case task_type::SNIFF_CHANNEL: {        
        pcap_args stop_pcap;
        stop_pcap.close = 1;
        sniffer_args_t stop_sniff;
        stop_sniff.stop = 1;
        do_sniffer_cmd(&stop_sniff);
        do_pcap_cmd(&stop_pcap);
        break;
    }
    default:
        break;
    }
}