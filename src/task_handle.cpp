#include "task_handle.h"
#include "sniffer.h"
#include "sd_adapter.h"
#include "netw_scan.h"

#include "esp_log.h"

const char *Task::TAG = "Task";

Task::Task() : duration(1), id(0), omode(outputMode::JSON_RESPONSE), tt(task_type::SLEEP) {
    ESP_LOGI(TAG, "ctor");    
}
Task::Task(int32_t mid, outputMode om, int32_t dur, task_type mtt) : duration(dur), id(mid),
        omode(om), tt(mtt) {
    ESP_LOGI(TAG, "ctor id omode"); 
}
Task::Task(Task &&task) : duration(task.duration), id(task.id),
        omode(task.omode), tt(task.tt) {
    ESP_LOGI(TAG, "move ctor");    
}

Task::Task(const Task &task) : duration(task.duration), id(task.id),
        omode(task.omode), tt(task.tt) {
    ESP_LOGI(TAG, "copy ctor");    
}

Task::~Task() {
    ESP_LOGI(TAG, "dtor");
}

std::string Task::generateJson() const {
    ESP_LOGI(TAG, "gen Json");
    return "";
}

void Task::parseJson(char *json) {
    ESP_LOGI(TAG, "parse Json");
}

std::map<std::string, std::string>& Task::get_params_map() {
    return params;
}

static sniffer_args_t parse_sniff_params(const std::map<std::string, std::string> &m) {
    sniffer_args_t res;
    res.stop = 0;
    if (m.find("channel") != m.end())
        res.channel = std::stoi(m.at("channel"));
    else
        res.channel = 1;
    if (m.find("count") != m.end())
        res.number = std::stoi(m.at("number"));
    else
        res.number = -1;
    return res;
}

void Task::do_task() {
    switch (tt)
    {
    case task_type::SNIFF_CHANNEL: {	
        pcap_args start_pcap;
        start_pcap.close = 0;
        start_pcap.file = "sniff_id" + std::to_string(id);
        start_pcap.open = 1;
        start_pcap.summary = 0;
        do_pcap_cmd(&start_pcap);
        sniffer_args_t start_sniff = parse_sniff_params(params);
        ESP_LOGI(TAG, "Channel %d", start_sniff.channel);
        do_sniffer_cmd(&start_sniff);
        break;
    }
    case task_type::SCAN_AP_ALL:
        wifi_netw_scan();
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