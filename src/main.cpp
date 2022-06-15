#include "sniffer.h"
#include "netw_scan.h"
#include "task_handle.h"
#include "state_handle.h"
#include "oled_disp.h"
#include "sim808.h"
#include "sd_adapter.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#include "ArduinoJson.h"

extern "C" {
#include "ssd1306.h"
//#include "font8x8_basic.h"
}

#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13

// WiFi defines
#define	WIFI_CHANNEL_MAX		(13)
#define	WIFI_CHANNEL_SWITCH_INTERVAL	(500)

static const char *TAG = "main";

extern "C" void app_main(void);
static esp_err_t event_handler(void *ctx, system_event_t *event);

static void initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

/* Initialize wifi with tcp/ip adapter */
static void initialize_wifi(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
}

static void main_loop_decorator(void *arg)
{
	StateMachine main_state_machine;
	main_state_machine.setup(DevConfigure{}); 
	
	main_state_machine.task_queue.emplace(new Task{1, outputMode::JSON_RESPONSE, 10, task_type::SCAN_AP_ALL});
	main_state_machine.task_queue.emplace(new Task{2, outputMode::JSON_RESPONSE, 20, task_type::SCAN_AP_ALL});
	main_state_machine.task_queue.emplace(new Task{3, outputMode::JSON_RESPONSE, 20, task_type::SCAN_AP_ALL});

	main_state_machine.main_loop();
}

xTaskHandle main_handle;

void app_main(void) {
	SSD1306_t dev;
	
	/* setup */
	initialize_nvs();
	initialize_wifi();  
	initialize_ssd1306(&dev);
	
	
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    
  	ESP_ERROR_CHECK(esp_wifi_start());// starts wifi usage
	// initialize_sim808Uart();
	// Test OLED
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, 0, "WiFISniffer\n", 11, false);

	//wifi_netw_scan_with_config(NULL);
	
	// init SD
	mount();

	pcap_args start_pcap;
	start_pcap.close = 0;
	start_pcap.file = "test_sniff";
	start_pcap.open = 1;
	start_pcap.summary = 0;

	pcap_args stop_pcap;
	stop_pcap.close = 1;
	stop_pcap.file = "test_sniff";
	stop_pcap.open = 0;
	stop_pcap.summary = 0;

	do_pcap_cmd(&start_pcap);
	vTaskDelay(10000 / portTICK_PERIOD_MS);
	do_pcap_cmd(&stop_pcap);
	for (;;) {
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
	//xTaskCreate(main_loop_decorator, "Main Task", 5*1024, NULL, 1, &main_handle);
/*
	StateMachine main_state_machine;
	main_state_machine.setup(DevConfigure{}); 
	
	main_state_machine.task_queue.emplace(new Task{1, outputMode::JSON_RESPONSE, 10, task_type::SCAN_AP_ALL});
	main_state_machine.task_queue.emplace(new Task{2, outputMode::JSON_RESPONSE, 20, task_type::SCAN_AP_ALL});
	main_state_machine.task_queue.emplace(new Task{3, outputMode::JSON_RESPONSE, 20, task_type::SCAN_AP_ALL});

	main_state_machine.main_loop();
*/
	//wifi_sniffer_init();
	
	// char oled_buffer[64] = {};
	// uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
	// /* loop */
	// while (true) {
	// 	//wifi_sniffer_set_channel(channel);
	// 	//wifi_netw_scan();
	// 	ESP_LOGI(TAG, "State: %s", main_state_machine.get_device_state().get_cstr());
	// 	strncpy(oled_buffer, main_state_machine.get_device_state().get_cstr(), 64);
	// 	ssd1306_display_text(&dev, 0, oled_buffer, strlen(oled_buffer), false);
	// 	//channel = (channel % WIFI_CHANNEL_MAX) + 1;

	// 	// Read data from the UART
    //     int len = uart_read_bytes(UART_SIM808_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
    //     // Write data back to the UART
    //     // uart_write_bytes(UART_SIM808_PORT_NUM, (const char *) data, len);
    //     if (len) {
    //         data[len] = '\0';
    //         ESP_LOGI(TAG, "Recv str: %s", (char *) data);
    //     }
	// 	vTaskDelay(2000 / portTICK_PERIOD_MS);
    // }
}