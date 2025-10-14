#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "led_controller.h"
#include "http_server.h"


#define WIFI_SSID       "esp32"
#define WIFI_PASSWORD	"1234567890"
static const char *TAG = "Main";

void app_main(void) {
    ESP_LOGI(TAG, "Starting ESP32-S3 LED Controller");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize components
    led_controller_init();
    wifi_manager_init();
    http_server_start();  // Start HTTP server
    
    ESP_LOGI(TAG, "System initialized. Waiting for WiFi...");

    // Main loop
    while (1) {
        // Connect to WiFi jika belum connected
        if (!wifi_manager_is_connected()) {
            ESP_LOGI(TAG, "Connecting to WiFi: %s", WIFI_SSID);
            wifi_manager_connect(WIFI_SSID, WIFI_PASSWORD);
            
            // Tunggu connection
            for (int i = 0; i < 10; i++) {
                if (wifi_manager_is_connected()) break;
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }

        // Jika WiFi connected
        if (wifi_manager_is_connected()) {
            static bool ip_printed = false;
            if (!ip_printed) {
                char ip_buffer[16];
                wifi_manager_get_ip(ip_buffer, sizeof(ip_buffer));
                ESP_LOGI(TAG, "✅ WiFi Connected!");
                ESP_LOGI(TAG, "📡 ESP32 IP: %s", ip_buffer);
                ESP_LOGI(TAG, "🌐 Send color commands to: http://%s/api/led", ip_buffer);
                
                // LED indicator connected
                led_controller_set_color(0, 255, 0); // Green
                vTaskDelay(pdMS_TO_TICKS(1000));
                led_controller_set_color(0, 0, 0);   // Off
                
                ip_printed = true;
            }
            
            // Tunggu 3 detik sebelum check lagi
            vTaskDelay(pdMS_TO_TICKS(3000));
            
        } else {
            // WiFi tidak connected
            ESP_LOGW(TAG, "WiFi not connected");
            
            // LED indicator disconnected (red blink)
            led_controller_set_color(255, 0, 0); // Red
            vTaskDelay(pdMS_TO_TICKS(500));
            led_controller_set_color(0, 0, 0);   // Off
            
            // Tunggu 1 detik sebelum reconnect
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}
