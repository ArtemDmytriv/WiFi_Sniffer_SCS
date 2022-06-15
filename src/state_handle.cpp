#include "state_handle.h"
#include "sniffer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// DevState class implementation
const char *DevState::TAG = "DevState";

DevState::DevState(device_state_type state, Task *ptr) : _s(state), task_to_do(ptr) { 
    ESP_LOGI(DevState::TAG, "ctor");
    if (this->task_to_do)
        ESP_LOGI(DevState::TAG, "Duartion State %d", this->task_to_do->duration);
}

// ?
DevState::DevState(const DevState &ds) : _s(ds._s), task_to_do(ds.task_to_do) { 
    ESP_LOGI(DevState::TAG, "copy ctor");
}

DevState::DevState(DevState &&ds) : _s(ds._s), task_to_do(ds.task_to_do) { 
    ESP_LOGI(DevState::TAG, "move ctor");
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
        task_to_do->do_task();
        vTaskDelay(50 / portTICK_PERIOD_MS);
        end = esp_log_timestamp();
        if ((end - start) > task_to_do->duration * 1000)
            break;
    }
    vTaskResume(main_handle);
}


// StateMachine class implementation
const char *StateMachine::TAG = "StateMachine";

StateMachine::StateMachine(DevState first_state) :
    _current_state(first_state)
{
    _current_config.value = 0; 
    ESP_LOGI(TAG, "ctor");
}

static int setup_module(int8_t devconf_num) {
    switch (devconf_num)
    {
    case DEV_OFFSET_WIFI_SNIFF: break;
    case DEV_OFFSET_WIFI_SCAN: break;
    case DEV_OFFSET_UART_SIM808: break;
    case DEV_OFFSET_I2C_SSD1306: break;
    case DEV_OFFSET_SPI_SD: break;
    case DEV_OFFSET_SLEEP_MODE: break;
    }
    return 0;
}

void StateMachine::setup(DevConfigure configure_setup) {
    //0111 1000
    //0010 0100
    for (int i = 0; i < sizeof(_current_config); i++) {
        // Need to disable some module
        if ( (configure_setup.value << i) & 0x01 && (~configure_setup.value << i) & 0x01) {
            ESP_LOGI(TAG, "Setup disable\n");
        }
        // Need to enable some module
        else {
            ESP_LOGI(TAG, "Setup enable\n");
        }
    }
};

static s_type get_state_for_task(const task_type &tt) {
    switch (tt)
    {
    case task_type::SCAN_AP_ALL : 
        return s_type::WIFI_SCAN;
    case task_type::SCAN_AP_PARAMS :
        return s_type::WIFI_SCAN;
    case task_type::SCAN_STA :
        return s_type::WIFI_SCAN;
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

static DevConfigure get_configure_for_state(const DevState &ds) {
    DevConfigure val;
    switch (ds.get_dev_state())
    {
    case s_type::INIT_STATE:
    case s_type::SLEEP:
        break;
    case s_type::WIFI_SCAN:
        val.isWifiScanMode = 1;
        break;
    case s_type::WIFI_SNIFFER:
        val.isWifiSniffMode = 1;
        val.isSPI_SD = 1;
        break;
    case s_type::GPRS_GET_TASK:
    case s_type::GPRS_POST_RESP:
        val.isUART_SIM808 = 1;
        break;
    case s_type::SD_CARD_GET:
        val.isSPI_SD = 1;
        break;
    }
    return val;
}

static void task_decorator(void *arg) {
    DevState *s = (DevState *)arg;
    for(;;) {
        if (s)
            s->do_job();
    }
} 

void StateMachine::main_loop() {
    ESP_LOGI(TAG, "main_loop job");
    do_job = false;
    DevState sleep_state = DevState{s_type::SLEEP};
    xTaskHandle xth;
    xTaskCreate(task_decorator, "Worker", 5*1024, &_current_state, 1, &xth);
    vTaskSuspend(xth);
    for (;;) {
        ESP_LOGI(TAG, "loop iter");
        if (!do_job && !task_queue.empty()) {
            ESP_LOGI(TAG, "Get Task from queue");
            Task *new_task = task_queue.front();
            task_queue.pop();
            ESP_LOGI(TAG, "Task Duration %d", new_task->duration);
            _current_state = DevState{get_state_for_task(new_task->get_task_type()), new_task};
            this->setup(get_configure_for_state(_current_state));
            do_job = true;
        }
        else if (!do_job) {
            _current_state = sleep_state;
        }

        if (do_job) {
            ESP_LOGI(TAG,"Start task");
            //_current_state.do_job();
            vTaskResume(xth);
            vTaskSuspend(NULL);
            ESP_LOGI(TAG,"End task");
            if (_current_state.get_dev_state() == s_type::WIFI_SNIFFER) {
                ESP_LOGI(TAG,"Disable sniffer");    
                wifi_sniffer_stop();
            }
            delete _current_state.get_task();
            vTaskSuspend(xth);
            do_job = false;
        }
        else {
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }

}