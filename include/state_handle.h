#ifndef _STATE_HANDLER_H
#define _STATE_HANDLER_H

#include "task_handle.h"
#include <queue>
#include <vector>

const std::vector<const char*> devstate2str_vec = {
    "Init state",
    "Wi-Fi Scan",
    "Wi-Fi Sniff",
    "GPRS Get request",
    "GPRS Post request",
    "SD card content view",
    "Sleep state"
};

enum DevConfigureOffset {
    DEV_OFFSET_WIFI_SNIFF  = 0,
    DEV_OFFSET_WIFI_SCAN   = 1,
    DEV_OFFSET_UART_SIM808 = 2,
    DEV_OFFSET_I2C_SSD1306 = 3,
    DEV_OFFSET_SPI_SD      = 4,
    DEV_OFFSET_SLEEP_MODE  = 5
};

union DevConfigure {
    unsigned char 
        isWifiSniffMode : 1,
        isWifiScanMode  : 1,
        isUART_SIM808   : 1,
        isI2C_SSD1306   : 1,
        isSPI_SD        : 1,
        isSleep         : 1,
        reserved1       : 1,
        reserved2       : 1;
    unsigned char value;
    DevConfigure(unsigned char value = 0) : value(value) {}
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
    void setup(DevConfigure configure_setup);
    void main_loop();

    DevConfigure get_device_conf_status() const {
        return _current_config;
    }
    DevState& get_device_state() {
        return _current_state;
    }
    const char* get_dev_state_cstr() const {
        return _current_state.get_cstr();
    }
    std::queue<Task*> task_queue;
private:
    DevConfigure _current_config;
    DevState _current_state;
    bool do_job;
    static const char *TAG; 
};

#endif // _STATE_HANDLER_H
