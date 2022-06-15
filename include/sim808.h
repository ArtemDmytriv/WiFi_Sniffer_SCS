#ifndef _SIM808_H
#define _SIM808_H

#define UART_SIM808_TXD (CONFIG_UART_SIM808_TXD)
#define UART_SIM808_RXD (CONFIG_UART_SIM808_RXD)
#define UART_SIM808_RTS (UART_PIN_NO_CHANGE)
#define UART_SIM808_CTS (UART_PIN_NO_CHANGE)

#define UART_SIM808_PORT_NUM   (CONFIG_UART_SIM808_PORT_NUM)
#define UART_SIM808_BAUD_RATE  (CONFIG_UART_SIM808_BAUD_RATE)
#define ECHO_TASK_STACK_SIZE   (CONFIG_EXAMPLE_TASK_STACK_SIZE)
#define BUF_SIZE (1024)

#include "task_handle.h"

#include <utility>
#include <string>
#include <vector>

#define SIM808_BUFSIZE 1024

enum class sim808_command {
    INIT_MODULE,
    INIT_GPRS,
    DEINIT_GPRS,
    GET_TASK_URL,
    AT_GET_GPS,
    SLEEP_MODE,
    WAKE_UP
};

// command + delays
const std::vector<std::pair<std::string, int32_t>> ATs_init_module = {
    {"AT\r\n", 300},
    {"AT0\r\n", 300},
    {"AT+CGNSPWR=1\r\n", 300},
    {"AT+ECHARGE=1\r\n", 300},
    {"AT&W\r\n", 300}
};

const std::vector<std::pair<std::string, int32_t>> ATs_deinit_GPRS = {
    {"AT+SAPBR=0,1\r\n", 300}
};

const std::vector<std::pair<std::string, int32_t>> ATs_init_GPRS = {
    {"AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n", 600},
    {"AT+SAPBR=3,1,\"APN\",\"internet\"\r\n", 100},
    {"AT+SAPBR=1,1\r\n", 300},
    {"AT+HTTPINIT\r\n", 300},
    {"AT+HTTPPARA=\"CID\",1\r\n", 100}
};

const std::vector<std::pair<std::string, int32_t>> ATs_get_GPS = {
    {"AT+CGNSSEQ=RMC\r\n", 300},
    {"AT+CGPSINF=0\r\n", 1000}
};

const std::vector<std::pair<std::string, int32_t>> ATs_GET_task = { 
    {"AT+HTTPPARA=\"URL\",\"http://34.230.43.26/task.json\"\r\n", 800}, 
    {"AT+HTTPACTION=0\r\n", 2000},
    {"AT+HTTPREAD\r\n", 2000}
};

const std::vector<std::pair<std::string, int32_t>> ATs_sleep = {
    {"AT+CSCLK=1\r\n", 1000}
};

const std::vector<std::pair<std::string, int32_t>> ATs_wake = {
    {"AT+CSCLK=0\r\n", 1000}
};

void initialize_sim808Uart(void);

Task * do_sim808_action(sim808_command action);

Task *get_task_sim808(void);

#endif // _SIM808_H
