#include "task_handle.h"
#include "sniffer.h"
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

void Task::parseJson(std::string json) {
    ESP_LOGI(TAG, "parse Json");
}

void Task::do_task() {
    switch (tt)
    {
    case task_type::SNIFF_CHANNEL:
        wifi_sniffer_init();
        wifi_sniffer_set_channel(1);
        wifi_sniffer_start();
        break;
    case task_type::SCAN_AP_ALL:
        wifi_netw_scan();
    case task_type::SLEEP:
        break;
    default:
        break;
    }
}