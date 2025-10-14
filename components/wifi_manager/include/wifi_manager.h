#pragma once
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

void wifi_manager_init(void);
void wifi_manager_connect(const char *ssid, const char *password);
bool wifi_manager_is_connected(void);
void wifi_manager_get_ip(char *ip_buffer, size_t buffer_size);
void wifi_manager_deinit(void);

#ifdef __cplusplus
}
#endif
