// Minimaler esp_log-Stub.
#ifndef ESP_LOG_STUB_H
#define ESP_LOG_STUB_H
#include <cstdio>
#define ESP_LOGI(tag, fmt, ...) do { printf("[I %s] " fmt "\n", tag, ##__VA_ARGS__); } while (0)
#define ESP_LOGW(tag, fmt, ...) do { printf("[W %s] " fmt "\n", tag, ##__VA_ARGS__); } while (0)
#define ESP_LOGE(tag, fmt, ...) do { printf("[E %s] " fmt "\n", tag, ##__VA_ARGS__); } while (0)
#endif
