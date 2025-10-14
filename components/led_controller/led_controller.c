#include "led_controller.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_RMT_RESOLUTION_HZ 10000000 // 10MHz
#define WS2812_PIN 48

static const char *TAG = "LED_Controller";
static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;
static rgb_color_t current_color = {0, 0, 0};

// Simple HSV to RGB conversion - FIXED version
static void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    // Ensure valid input ranges
    h = h % 360;
    s = (s > 100) ? 100 : s;
    v = (v > 100) ? 100 : v;
    
    float hue = h / 60.0f;
    float saturation = s / 100.0f;
    float value = v / 100.0f;
    
    int i = (int)hue;
    float f = hue - i;
    float p = value * (1 - saturation);
    float q = value * (1 - saturation * f);
    float t = value * (1 - saturation * (1 - f));
    
    float red, green, blue;
    
    switch (i) {
        case 0: red = value; green = t; blue = p; break;
        case 1: red = q; green = value; blue = p; break;
        case 2: red = p; green = value; blue = t; break;
        case 3: red = p; green = q; blue = value; break;
        case 4: red = t; green = p; blue = value; break;
        case 5: red = value; green = p; blue = q; break;
        default: red = 0; green = 0; blue = 0; break;
    }
    
    *r = (uint8_t)(red * 255);
    *g = (uint8_t)(green * 255);
    *b = (uint8_t)(blue * 255);
}

// Simplified RMT encoder setup
static esp_err_t setup_rmt_encoder(rmt_encoder_handle_t *encoder) {
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 3, // 0.3us
            .level1 = 0,
            .duration1 = 9, // 0.9us
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 9, // 0.9us
            .level1 = 0,
            .duration1 = 3, // 0.3us
        },
        .flags.msb_first = 1
    };
    return rmt_new_bytes_encoder(&bytes_encoder_config, encoder);
}

void led_controller_init(void) {
    ESP_LOGI(TAG, "Initializing LED Controller for ESP32-S3");
    
    // Initialize RMT channel
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = WS2812_PIN,
        .mem_block_symbols = 64,
        .resolution_hz = LED_RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    
    esp_err_t ret = rmt_new_tx_channel(&tx_chan_config, &led_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT channel: %s", esp_err_to_name(ret));
        return;
    }
    
    // Setup encoder
    ret = setup_rmt_encoder(&led_encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT encoder: %s", esp_err_to_name(ret));
        rmt_del_channel(led_chan);
        led_chan = NULL;
        return;
    }
    
    // Enable channel
    ret = rmt_enable(led_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable RMT channel: %s", esp_err_to_name(ret));
        rmt_del_encoder(led_encoder);
        rmt_del_channel(led_chan);
        led_chan = NULL;
        led_encoder = NULL;
        return;
    }
    
    // Turn off LED initially
    led_controller_set_color(0, 0, 0);
    ESP_LOGI(TAG, "LED Controller initialized successfully");
}

void led_controller_set_color(uint8_t red, uint8_t green, uint8_t blue) {
    if (led_chan == NULL || led_encoder == NULL) {
        ESP_LOGW(TAG, "LED controller not initialized, skipping set_color");
        return;
    }

    current_color.red = red;
    current_color.green = green;
    current_color.blue = blue;
    
    // WS2812 uses GRB format
    uint8_t led_data[] = {green, red, blue};
    
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };
    
    esp_err_t ret = rmt_transmit(led_chan, led_encoder, led_data, sizeof(led_data), &tx_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to transmit LED data: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = rmt_tx_wait_all_done(led_chan, pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wait for LED transmission: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "LED color set to R:%d G:%d B:%d", red, green, blue);
}

void led_controller_set_color_hsv(uint16_t hue, uint8_t saturation, uint8_t value) {
    uint8_t red, green, blue;
    hsv_to_rgb(hue, saturation, value, &red, &green, &blue);
    led_controller_set_color(red, green, blue);
    ESP_LOGI(TAG, "LED HSV set to H:%d S:%d V:%d -> R:%d G:%d B:%d", 
             hue, saturation, value, red, green, blue);
}

void led_controller_get_current_color(rgb_color_t *color) {
    if (color) {
        *color = current_color;
    }
}

void led_controller_deinit(void) {
    if (led_encoder) {
        rmt_del_encoder(led_encoder);
        led_encoder = NULL;
    }
    if (led_chan) {
        rmt_disable(led_chan);
        rmt_del_channel(led_chan);
        led_chan = NULL;
    }
    ESP_LOGI(TAG, "LED Controller deinitialized");
}
