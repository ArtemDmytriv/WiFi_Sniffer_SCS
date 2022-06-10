#include "sniffer.h"
#include "netw_scan.h"

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#define	LED_GPIO_PIN			GPIO_NUM_2
#define	WIFI_CHANNEL_MAX		(13)
#define	WIFI_CHANNEL_SWITCH_INTERVAL	(500)

static const char *TAG = "main";

enum class state {
	SCAN_ALL_AP,
	SCAN_CHANNEL_AP,
	SCAN_AP_FOR_STA,
	SNIFF_CHANNEL,
	SNIFF_STA,
	GET_TASK,
	POST_RESP
};

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
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
}

void app_main(void) {
	uint8_t level = 0, channel = 1;

	/* setup */
	initialize_nvs();
	initialize_wifi();  
	
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    
	//wifi_sniffer_init();
	gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);
	
  	ESP_ERROR_CHECK(esp_wifi_start());// starts wifi usage
	/* loop */
	while (true) {
		gpio_set_level(LED_GPIO_PIN, level ^= 1);
		vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS);
		
		//wifi_sniffer_set_channel(channel);
		wifi_netw_scan();
		ESP_LOGI(TAG, "Iteration");
		channel = (channel % WIFI_CHANNEL_MAX) + 1;
    }
}

esp_err_t event_handler(void *ctx, system_event_t *event) {
	
	return ESP_OK;
}