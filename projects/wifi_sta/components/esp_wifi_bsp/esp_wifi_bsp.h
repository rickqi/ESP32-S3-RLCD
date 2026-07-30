#ifndef ESP_WIFI_BSP_H
#define ESP_WIFI_BSP_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"  //WIFI

#ifdef __cplusplus
extern "C" {
#endif

void espwifi_Init(void);

#ifdef __cplusplus
}
#endif


#endif