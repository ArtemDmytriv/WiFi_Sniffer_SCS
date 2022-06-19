#include "sniffer.h"
#include "netw_scan.h"
#include "task_handle.h"
#include "state_handle.h"
#include "oled_disp.h"
#include "sim808.h"
#include "sd_adapter.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

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

// WiFi defines
#define	WIFI_CHANNEL_MAX		(13)
#define	WIFI_CHANNEL_SWITCH_INTERVAL	(500)
#define SSD1306_LINE_LEN (17)
static const char *TAG = "main";
SSD1306_t dev;

extern "C" void app_main(void);

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

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

  	ESP_ERROR_CHECK(esp_wifi_start());// starts wifi usage
}

xSemaphoreHandle task_sem;
xTaskHandle main_handle;
xTaskHandle task_getter_handle;

StateMachine *pmain_state_machine = nullptr;

static void task_gprs_getter(void *arg) {
	vTaskSuspend(NULL);
	do_sim808_action(sim808_command::INIT_GPRS, pmain_state_machine->task_queue);
	for(;;) {
		ESP_LOGI("gprs_getter", LOG_COLOR(LOG_COLOR_BROWN) "Waiting Semaphore..." LOG_RESET_COLOR);
		if (xSemaphoreTake(task_sem, ( TickType_t ) 10) == pdTRUE) {
			ESP_LOGI("gprs_getter", LOG_COLOR(LOG_COLOR_BROWN) "Take Semaphore" LOG_RESET_COLOR);

			ssd1306_display_text(&dev, 3, "SIM808", sizeof("SIM808"), false);
			do_sim808_action(sim808_command::GET_TASK_URL, pmain_state_machine->task_queue);

			ssd1306_clear_line(&dev, 3, false);
			ESP_LOGI("gprs_getter", LOG_COLOR(LOG_COLOR_BROWN) "Give Semaphore" LOG_RESET_COLOR);
			xSemaphoreGive(task_sem);		
			vTaskSuspend(NULL);
		}
		vTaskDelay(10000 / portTICK_PERIOD_MS);
	}
}


static void main_loop_decorator(void *arg)
{
	StateMachine main_state_machine;
	pmain_state_machine = &main_state_machine;
	main_state_machine.main_loop();
}

void app_main(void) {

	ESP_LOGI(TAG, "Start app_main");
	/* setup */
	initialize_nvs();
	initialize_wifi();
	initialize_ssd1306(&dev);
	initialize_sim808Uart();
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, 0, "--WiFi-Sniffer--", 17, false);
	ssd1306_display_text(&dev, 1, "----------------", 17, false);

	// init SD
	mount_sd();
	create_wifi_filter_hashtable();
	vTaskDelay(2000 / portTICK_PERIOD_MS);
    task_sem = xSemaphoreCreateMutex();
	xTaskCreate(task_gprs_getter, "Task getter", 3*1024, NULL, 1, &task_getter_handle);
	xTaskCreate(main_loop_decorator, "Main Loop Task", 5*1024, NULL, 1, &main_handle);
}