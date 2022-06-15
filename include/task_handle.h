#ifndef _TASK_HANDLE_H
#define _TASK_HANDLE_H

#include <map>
#include <string>

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
    std::string generateJson() const;
    void parseJson(char *);
    void do_task();
    void stop_task();
    std::map<std::string, std::string>& get_params_map();
    task_type get_task_type() const {
        return tt;
    }
    int32_t duration;
    int32_t id;
private:
    outputMode omode;
    task_type tt;
    std::map<std::string, std::string> params;
    
    static const char *TAG; 
};

#endif // _TASK_HANDLE_H
