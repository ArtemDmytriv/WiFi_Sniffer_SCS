#include "state_handle.h"
#include "sniffer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

// DevState class implementation
const char *DevState::TAG = "DevState";

DevState::DevState(device_state_type state, Task *ptr) : _s(state), task_to_do(ptr) { 
    ESP_LOGI(DevState::TAG, "ctor");
}

// ?
DevState::DevState(const DevState &ds) : _s(ds._s), task_to_do(ds.task_to_do) { 
}

DevState::DevState(DevState &&ds) : _s(ds._s), task_to_do(ds.task_to_do) { 
    ds.task_to_do = nullptr;
}


DevState& DevState::operator=(const DevState &state) {
    _s = state._s;
    task_to_do = state.task_to_do;
    return *this;
}

DevState::~DevState() { 
    ESP_LOGI(DevState::TAG, "dtor");
}

void DevState::change_state(device_state_type state) {
    ESP_LOGI(DevState::TAG, "change state job");
    _s = state;
}

void DevState::do_job() {
    uint32_t start = esp_log_timestamp(),
                end = 0;
    ESP_LOGI(DevState::TAG, "Do some job, start Tick %d", start);
    task_to_do->do_task();
    for(;;) {
        if (_s != s_type::WIFI_SNIFFER)
            task_to_do->do_task();
        vTaskDelay(50 / portTICK_PERIOD_MS);
        end = esp_log_timestamp();
        if ((end - start) > task_to_do->duration * 1000)
            break;
    }
    //vTaskResume(main_handle);
    //vTaskSuspend(NULL);
}

void DevState::remove_task() {
    if (task_to_do) {
        delete task_to_do;
    }
    task_to_do = nullptr;
}

// StateMachine class implementation
const char *StateMachine::TAG = "StateMachine";

StateMachine::StateMachine(DevState first_state) :
    _current_state(first_state)
{
    ESP_LOGI(TAG, "ctor");
}


static s_type get_state_for_task(const task_type &tt) {
    switch (tt)
    {
    case task_type::SCAN_AP_ALL : 
        return s_type::WIFI_SCAN;
    case task_type::SCAN_AP_PARAMS :
        return s_type::WIFI_SCAN;
    case task_type::SCAN_STA :
        return s_type::WIFI_SNIFFER;
    case task_type::SNIFF_CHANNEL :
        return s_type::WIFI_SNIFFER;
    case task_type::SNIFF_STA :
        return s_type::WIFI_SNIFFER;
    case task_type::GET_TASK :
        return s_type::GPRS_GET_TASK;
    case task_type::POST_RESP :
        return s_type::GPRS_POST_RESP;
    case task_type::SLEEP :
        return s_type::SLEEP;
    }
    return s_type::SLEEP;
}


void StateMachine::main_loop() {
    ESP_LOGI(TAG, "main_loop job");
    bool need_do_task = false;

    for (;;) {
        ESP_LOGI(TAG, "Idle");
        if (!need_do_task && !task_queue.empty()) { // get task if "task_getter" sleep and queue not empty
            if (xSemaphoreTake(task_sem, ( TickType_t ) 10) == pdTRUE) {
                Task *new_task = task_queue.front();
                ESP_LOGI(TAG, "Get Task from queue %d", new_task->id);
                task_queue.pop();
                ESP_LOGI(TAG, "Task Duration %d", new_task->duration);
                _current_state = std::move(DevState{get_state_for_task(new_task->get_task_type()), new_task});
                need_do_task = true;
                xSemaphoreGive(task_sem);
            }
        }

        if (need_do_task) { // if we take task from queue
            vTaskSuspend(task_getter_handle);
            ESP_LOGI(TAG,"Start task");
            _current_state.do_job(); // do task

            ESP_LOGI(TAG,"End task");
            _current_state.get_task()->stop_task(); // stop if task not trigger job by itself
            _current_state.remove_task();

            need_do_task = false;
        }
        else {
            vTaskResume(task_getter_handle);
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
    }

}