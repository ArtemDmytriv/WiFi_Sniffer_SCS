#include "sim808.h"

#include "esp_log.h"
#include "driver/uart.h"

static const char *TAG = "sim808uart";

void get_response(char *buffer) {
	int len = uart_read_bytes(UART_SIM808_PORT_NUM, buffer, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
		
	if (len) {
		//const int line_length = 17;
		//char line[17] = {};
		buffer[len] = '\0';
		// for (int i = 0, j = 0; i < len; j++, i += line_length) {
		// 	snprintf(line, line_length - 1, "%s", (char*)data + i);
		// 	ssd1306_display_text(&dev, j % 8, line, line_length, false);
		// }
		ESP_LOGI(TAG, "Recv str: %d %s", len, (char *) buffer);
	}
}

Task *do_sim808_action(sim808_command action) {
	const std::vector<std::pair<std::string, int32_t>> *ptr_vec = &ATs_sleep;
	switch (action) {
		case sim808_command::INIT_MODULE :
			initialize_sim808Uart();
			return  nullptr;
		case sim808_command::INIT_GPRS :
			ptr_vec = &ATs_init_GPRS;
			break;
		case sim808_command::DEINIT_GPRS :
			ptr_vec = &ATs_deinit_GPRS;
			break;
		case sim808_command::GET_TASK_URL :
			return get_task_sim808();
		case sim808_command::SLEEP_MODE :
			ptr_vec = &ATs_sleep;
			break;
		case sim808_command::AT_GET_GPS:
			ESP_LOGI(TAG, "GPS not implemented");
			return nullptr;
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
	return nullptr;
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

    ESP_ERROR_CHECK(uart_driver_install(UART_SIM808_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
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

Task *get_task_sim808(void) {
	Task *new_task = new Task{10, outputMode::SD_CARD, 10, task_type::SNIFF_CHANNEL};
	char *buffer = (char*)malloc(SIM808_BUFSIZE);
	ESP_LOGI(TAG, "get_task_sim808");
	for (int i = 0; i < ATs_GET_task.size(); i++) {
		uart_write_bytes(UART_SIM808_PORT_NUM, ATs_GET_task[i].first.c_str(), ATs_GET_task[i].first.size());
		vTaskDelay(ATs_GET_task[i].second / portTICK_PERIOD_MS);
		// read response
		get_response(buffer);
	}
	new_task->parseJson(buffer);
	free(buffer);
	return new_task;
}
