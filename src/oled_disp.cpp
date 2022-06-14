#include "oled_disp.h"
#include "esp_log.h"

static const char *TAG = "oled1306";

void initialize_ssd1306(SSD1306_t *dev) {

	ESP_LOGI(TAG, "INTERFACE is i2c");
	ESP_LOGI(TAG, "CONFIG_SDA_GPIO=%d", CONFIG_SDA_GPIO);
	ESP_LOGI(TAG, "CONFIG_SCL_GPIO=%d", CONFIG_SCL_GPIO);
	ESP_LOGI(TAG, "CONFIG_RESET_GPIO=%d", CONFIG_RESET_GPIO);
	i2c_master_init(dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);

	ESP_LOGI(TAG, "Panel is 128x64");
	ssd1306_init(dev, 128, 64);

	ssd1306_clear_screen(dev, false);
}