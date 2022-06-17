#ifndef _STATE_HANDLER_H
#define _STATE_HANDLER_H

#include "task_handle.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <queue>
#include <vector>

extern xSemaphoreHandle task_sem;
extern xTaskHandle main_handle;
extern xTaskHandle task_getter_handle;

const std::vector<const char*> devstate2str_vec = {
    "Init state",
    "Wi-Fi Scan",
    "Wi-Fi Sniff",
    "GPRS Get request",
    "GPRS Post request",
    "SD card content view",
    "Sleep state"
};

class DevState {
public:
    enum class device_state_type {
        INIT_STATE,
        WIFI_SCAN,
        WIFI_SNIFFER,
        GPRS_GET_TASK,
        GPRS_POST_RESP,
        SD_CARD_GET,
        SLEEP
    };
    DevState(device_state_type state, Task *ptr = nullptr);
    DevState(const DevState &state);
    DevState(DevState &&state);
    DevState& operator=(const DevState &state);
    ~DevState();
    void do_job();
    void remove_task();
    void set_task(Task *task) {
        task_to_do = task;
    }
    void change_state(device_state_type);
    device_state_type get_dev_state() const {
        return _s;
    }
    const char *get_cstr() const {
        return devstate2str_vec[static_cast<int>(_s)];
    }
    Task* get_task() const {
        return task_to_do;
    }
private:
    device_state_type _s;
    Task *task_to_do;
    static const char *TAG; 
};

using s_type = DevState::device_state_type;

class StateMachine {
public:

    StateMachine(DevState first_state = DevState{s_type::INIT_STATE});
    void main_loop();

    DevState& get_device_state() {
        return _current_state;
    }
    const char* get_dev_state_cstr() const {
        return _current_state.get_cstr();
    }
    std::queue<Task*> task_queue;
private:
    DevState _current_state;
    bool do_job;
    static const char *TAG; 
};

#endif // _STATE_HANDLER_H
