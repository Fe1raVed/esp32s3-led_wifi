#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include "led_controller.h"

static const char *TAG = "HTTP_Server";
static httpd_handle_t server = NULL;

// Handler untuk menerima warna RGB
static esp_err_t led_color_handler(httpd_req_t *req) {
    char buffer[100];
    int ret = httpd_req_recv(req, buffer, sizeof(buffer) - 1);
    
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive data");
        return ESP_FAIL;
    }
    buffer[ret] = '\0';
    
    ESP_LOGI(TAG, "Received: %s", buffer);
    
    // Parse JSON
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *red = cJSON_GetObjectItem(json, "red");
    cJSON *green = cJSON_GetObjectItem(json, "green");
    cJSON *blue = cJSON_GetObjectItem(json, "blue");
    
    if (cJSON_IsNumber(red) && cJSON_IsNumber(green) && cJSON_IsNumber(blue)) {
        // Set LED color
        led_controller_set_color(red->valueint, green->valueint, blue->valueint);
        
        // Response success
        httpd_resp_sendstr(req, "{\"status\":\"success\",\"message\":\"LED color updated\"}");
        ESP_LOGI(TAG, "LED color set to R:%d G:%d B:%d", red->valueint, green->valueint, blue->valueint);
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing color values");
    }
    
    cJSON_Delete(json);
    return ESP_OK;
}

// Handler untuk menerima warna HSV
static esp_err_t led_hsv_handler(httpd_req_t *req) {
    char buffer[100];
    int ret = httpd_req_recv(req, buffer, sizeof(buffer) - 1);
    
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive data");
        return ESP_FAIL;
    }
    buffer[ret] = '\0';
    
    ESP_LOGI(TAG, "Received HSV: %s", buffer);
    
    // Parse JSON
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *hue = cJSON_GetObjectItem(json, "hue");
    cJSON *saturation = cJSON_GetObjectItem(json, "saturation");
    cJSON *value = cJSON_GetObjectItem(json, "value");
    
    if (cJSON_IsNumber(hue) && cJSON_IsNumber(saturation) && cJSON_IsNumber(value)) {
        // Set LED color using HSV
        led_controller_set_color_hsv(hue->valueint, saturation->valueint, value->valueint);
        
        // Response success
        httpd_resp_sendstr(req, "{\"status\":\"success\",\"message\":\"LED HSV color updated\"}");
        ESP_LOGI(TAG, "LED HSV set to H:%d S:%d V:%d", hue->valueint, saturation->valueint, value->valueint);
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing HSV values");
    }
    
    cJSON_Delete(json);
    return ESP_OK;
}

// URI handlers
static const httpd_uri_t led_color_uri = {
    .uri       = "/api/led",
    .method    = HTTP_POST,
    .handler   = led_color_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t led_hsv_uri = {
    .uri       = "/api/led/hsv", 
    .method    = HTTP_POST,
    .handler   = led_hsv_handler,
    .user_ctx  = NULL
};

void http_server_start(void) {
    ESP_LOGI(TAG, "Starting HTTP Server");
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 10;
    
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &led_color_uri);
        httpd_register_uri_handler(server, &led_hsv_uri);
        ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
    } else {
        ESP_LOGE(TAG, "Failed to start HTTP server");
    }
}

void http_server_stop(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
        ESP_LOGI(TAG, "HTTP server stopped");
    }
}
