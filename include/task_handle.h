#ifndef _TASK_HANDLE_H
#define _TASK_HANDLE_H

#include <map>
#include <string>
#include "ArduinoJson.h"

enum class outputMode {
    SD_CARD,
    JSON_RESPONSE
};

enum class task_type {
    SCAN_AP_ALL,
    SCAN_AP_PARAMS,
    SCAN_STA,
    SNIFF_CHANNEL,
    SNIFF_STA,
    GET_TASK,
    POST_RESP,
    SLEEP
};

class Task {
public:
    Task();
    Task(int32_t id, outputMode om, int32_t dur, task_type tt);
    Task(Task &&task);
    Task(const Task &task);
    ~Task();
    void do_task();
    void stop_task();
    void parseJson_parameters(const JsonObject &ref);
    void* get_params() {
        return params;
    }
    task_type get_task_type() const {
        return tt;
    }
    void set_task_type(task_type mtt);
    int32_t duration;
    int32_t id;
private:
    outputMode omode;
    task_type tt;
    std::string filename;
    void *params;
    
    static const char *TAG; 
};

#endif // _TASK_HANDLE_H
