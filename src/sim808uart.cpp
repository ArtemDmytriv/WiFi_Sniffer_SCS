#include "sim808.h"

#include "esp_log.h"
#include "driver/uart.h"

static const char *TAG = "sim808uart";

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

    ESP_ERROR_CHECK(uart_driver_install(UART_SIM808_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_SIM808_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_SIM808_PORT_NUM, UART_SIM808_TXD, UART_SIM808_RXD, UART_SIM808_RTS, UART_SIM808_CTS));
}

void setup_Sim808(void) {

}