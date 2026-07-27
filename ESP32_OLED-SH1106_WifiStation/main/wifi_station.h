#ifndef WIFI_STATION_H
#define WIFI_STATION_H

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Wi-Fi credentials requested for this project.
 * For a production device, credentials should normally be stored in NVS,
 * provisioned at runtime, or supplied through menuconfig instead of being
 * committed directly to source code.
 */
#define WIFI_STATION_SSID                 "YOUR_WIFI_NAME"
#define WIFI_STATION_PASSWORD             "YOUR_WIFI_PASSWORD"
#define WIFI_STATION_MAXIMUM_RETRY         10
#define WIFI_STATION_IP_STRING_LENGTH      16

/**
 * @brief Initialize Wi-Fi station mode and wait for an IPv4 address.
 *
 * This function initializes NVS, ESP-NETIF, the default event loop, and the
 * Wi-Fi station interface. It blocks until the station obtains an IPv4 address
 * or the configured retry limit is reached.
 *
 * @param ip_address Output buffer that receives an IPv4 string such as
 *                   "192.168.1.100".
 * @param ip_address_size Size of the output buffer. Use at least
 *                        WIFI_STATION_IP_STRING_LENGTH bytes.
 *
 * @return ESP_OK when connected and an IP address was obtained.
 * @return ESP_ERR_INVALID_ARG when the output buffer is invalid.
 * @return ESP_ERR_INVALID_SIZE when the output buffer is too small.
 * @return ESP_FAIL when the retry limit is reached.
 * @return Another ESP-IDF error code when initialization fails.
 */
esp_err_t wifi_station_connect(char *ip_address, size_t ip_address_size);

/**
 * @brief Return true when the station currently has an IPv4 address.
 */
bool wifi_station_is_connected(void);

/**
 * @brief Return the most recently assigned IPv4 address as a string.
 *
 * The returned pointer remains valid for the lifetime of the application.
 */
const char *wifi_station_get_ip_address(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_STATION_H */
