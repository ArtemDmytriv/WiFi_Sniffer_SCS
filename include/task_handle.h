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
    void parseJson(std::string);
    void do_task();
    task_type get_task_type() const {
        return tt;
    }
private:
    int32_t id;
    outputMode omode;
    int32_t duration;
    task_type tt;
    std::map<std::string, std::string> params;
    
    static const char *TAG; 
};

#endif // _TASK_HANDLE_H
