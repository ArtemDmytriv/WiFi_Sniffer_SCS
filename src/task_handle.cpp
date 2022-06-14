#include "task_handle.h"
#include "sniffer.h"

#include "esp_log.h"

const char *Task::TAG = "Task";

Task::Task() : id(0), omode(outputMode::JSON_RESPONSE) {
    ESP_LOGI(TAG, "ctor");    
}
Task::Task(int32_t mid, outputMode om, int32_t dur, task_type mtt) : id(mid), omode(om), duration(dur), tt(mtt) {
    ESP_LOGI(TAG, "ctor id omode"); 
}
Task::Task(Task &&task) {
    ESP_LOGI(TAG, "move ctor");    
}

Task::Task(const Task &task) {
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
    ESP_LOGI(TAG, "do_task");
    switch (tt)
    {
    case task_type::SNIFF_CHANNEL:
        
        break;
    
    default:
        break;
    }
}