#include "sim808.h"

#include "esp_log.h"
#include "driver/uart.h"
#include <queue>
#include <sstream>
#include "ArduinoJson.h"

static const char *TAG = "sim808uart";

void get_response(char *buffer) {
	int len = uart_read_bytes(UART_SIM808_PORT_NUM, buffer, (SIM808_BUFSIZE - 1), 20 / portTICK_PERIOD_MS);

	if (len) {
		buffer[len] = '\0';
		ESP_LOGI(TAG, "Recv str: %d %s", len, (char *) buffer);
	}
}

static int32_t max_task_id = -1;

static task_type str2type_task(const std::string &str) {
	if (str == "sniffer")
		return task_type::SNIFF_CHANNEL;
	if (str == "sniffer_sta")
		return task_type::SNIFF_STA;
	if (str == "scan_all")
		return task_type::SCAN_AP_ALL;
	if (str == "scan_sta")
		return task_type::SCAN_STA;
	if (str == "scan")
		return task_type::SCAN_AP_PARAMS;
	return task_type::SLEEP;
}

static int32_t parseJson(char* json, std::queue<Task *> &queue) {
	DynamicJsonDocument doc(512);
	deserializeJson(doc, json);
	JsonArray arr = doc.as<JsonArray>();
	int k = 0;
	ESP_LOGI(TAG, "Parse json");
	for (JsonObject repo : arr) {
		if (repo.containsKey("id") && repo.containsKey("task")) {
			int id = repo["id"].as<int>();
			if (id < 0 || id > max_task_id) {
				max_task_id = (id > max_task_id? id : max_task_id);
	
				Task *new_task = new Task{id, outputMode::SD_CARD, repo["duration"] | 10, task_type::SLEEP};
				std::string task_type_val = repo["task"].as<const char*>();
				new_task->set_task_type(str2type_task(task_type_val));
				new_task->parseJson_parameters(repo);
				
				ESP_LOGI(TAG, "Add task id=%d to queue", id);
				queue.push(new_task);
				k++;
			}
			else {			
				ESP_LOGI(TAG, "No need to add Task id=%d, was added earlier", id);
			}
		}
	}
	return k;
}

static int get_task_sim808(std::queue<Task *> &queue) {
	char *buffer = (char*)malloc(SIM808_BUFSIZE);
	ESP_LOGI(TAG, "get_task_sim808");
	for (int i = 0; i < ATs_GET_task.size(); i++) {
		uart_write_bytes(UART_SIM808_PORT_NUM, ATs_GET_task[i].first.c_str(), ATs_GET_task[i].first.size());
		vTaskDelay(ATs_GET_task[i].second / portTICK_PERIOD_MS);
		// read response
		get_response(buffer);
	}

	int count_added = parseJson(strchr(buffer, '['), queue);
	free(buffer);
	return count_added;
}


void do_sim808_action(sim808_command action, std::queue<Task *> &pqueue) {
	const std::vector<std::pair<std::string, int32_t>> *ptr_vec = &ATs_sleep;
	switch (action) {
		case sim808_command::INIT_MODULE :
			initialize_sim808Uart();
			return;
		case sim808_command::INIT_GPRS :
			ptr_vec = &ATs_init_GPRS;
			break;
		case sim808_command::DEINIT_GPRS :
			ptr_vec = &ATs_deinit_GPRS;
			break;
		case sim808_command::GET_TASK_URL : {
			int count_added = get_task_sim808(pqueue);
			std::string oled_buffer = "Added " + std::to_string(count_added) + " tasks";
			ssd1306_display_text(&dev, 4, oled_buffer.c_str(), oled_buffer.size(), false);
			return;
		}
		case sim808_command::SLEEP_MODE :
			ptr_vec = &ATs_sleep;
			break;
		case sim808_command::AT_GET_GPS:
			ESP_LOGI(TAG, "GPS not implemented");
			return;
		case sim808_command::WAKE_UP :
			ptr_vec = &ATs_wake;
			break;
	}
	char *buffer = (char*)malloc(SIM808_BUFSIZE);

	for (int i = 0; i < ptr_vec->size(); i++) {
		uart_write_bytes(UART_SIM808_PORT_NUM, (*ptr_vec)[i].first.c_str(), (*ptr_vec)[i].first.size());
		vTaskDelay((*ptr_vec)[i].second / portTICK_PERIOD_MS);
		// read response
		get_response(buffer);
	}
	free(buffer);
	return;
}

void initialize_sim808Uart(void) {
	ESP_LOGI(TAG, "SIM808 INTERFACE is UART");
	ESP_LOGI(TAG, "CONFIG_TXD_GPIO=%d", UART_SIM808_TXD);
	ESP_LOGI(TAG, "CONFIG_RXD_GPIO=%d", UART_SIM808_RXD);
		uart_config_t uart_config = {
		UART_SIM808_BAUD_RATE, 		// .baud_rate
		UART_DATA_8_BITS, 			// .data_bits
		UART_PARITY_DISABLE,		// .parity
		UART_STOP_BITS_1,			// .stop_bits
		UART_HW_FLOWCTRL_DISABLE,	// .flow_ctrl
		0,							// .rx_flow_ctrl_thresh
		UART_SCLK_APB				// .source_clk
	};

    int intr_alloc_flags = 0;

	char *buffer = (char*)malloc(SIM808_BUFSIZE);

    ESP_ERROR_CHECK(uart_driver_install(UART_SIM808_PORT_NUM, SIM808_BUFSIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_SIM808_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_SIM808_PORT_NUM, UART_SIM808_TXD, UART_SIM808_RXD, UART_SIM808_RTS, UART_SIM808_CTS));

	for (int i = 0; i < ATs_init_module.size(); i++) {
		uart_write_bytes(UART_SIM808_PORT_NUM, ATs_init_module[i].first.c_str(), ATs_init_module[i].first.size());
		vTaskDelay(ATs_init_module[i].second / portTICK_PERIOD_MS);
		// read response
		get_response(buffer);
	}
	free(buffer);
}
