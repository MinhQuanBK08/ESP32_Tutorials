#include <stdio.h>
#include <string.h>

#include "driver/i2c.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_demos.h"
#include "mochi_faces.h"
#include "sh1106.h"
#include "wifi_station.h"

/*
 * I2C pins requested for this project.
 * Some OLED modules label the I2C clock pin as SCK instead of SCL.
 */
#define APP_I2C_SDA_GPIO              23
#define APP_I2C_SCL_GPIO              22
#define APP_I2C_PORT                  I2C_NUM_0
#define APP_I2C_FREQUENCY_HZ       400000
#define APP_SH1106_I2C_ADDRESS      0x3C

static const char *TAG = "main";

/**
 * @brief Configure and install the legacy ESP-IDF I2C master driver.
 *
 * This uses driver/i2c.h and is suitable for ESP-IDF 4.4 as well as ESP-IDF
 * 5.x projects that still use the legacy I2C API.
 */
static esp_err_t app_i2c_master_init(void)
{
    i2c_config_t configuration = {0};

    configuration.mode = I2C_MODE_MASTER;
    configuration.sda_io_num = APP_I2C_SDA_GPIO;
    configuration.scl_io_num = APP_I2C_SCL_GPIO;
    configuration.sda_pullup_en = GPIO_PULLUP_ENABLE;
    configuration.scl_pullup_en = GPIO_PULLUP_ENABLE;
    configuration.master.clk_speed = APP_I2C_FREQUENCY_HZ;

    esp_err_t result = i2c_param_config(APP_I2C_PORT, &configuration);

    if (result != ESP_OK) {
        ESP_LOGE(TAG,
                 "i2c_param_config() failed: %s",
                 esp_err_to_name(result));
        return result;
    }

    result = i2c_driver_install(APP_I2C_PORT,
                                I2C_MODE_MASTER,
                                0,
                                0,
                                0);

    if (result == ESP_ERR_INVALID_STATE) {
        /*
         * The port may already have been installed by another component.
         * In that case, continue using the existing driver.
         */
        ESP_LOGW(TAG, "I2C driver is already installed");
        return ESP_OK;
    }

    return result;
}

/**
 * @brief Draw the Wi-Fi connection screen before starting the station.
 */
static esp_err_t app_show_wifi_connecting(sh1106_t *display)
{
    sh1106_clear(display, SH1106_COLOR_BLACK);

    sh1106_draw_rectangle(display,
                          0,
                          0,
                          SH1106_WIDTH,
                          SH1106_HEIGHT,
                          SH1106_COLOR_WHITE);

    sh1106_draw_string(display,
                       17,
                       8,
                       "WIFI STATION",
                       SH1106_COLOR_WHITE);

    sh1106_draw_line(display,
                     4,
                     19,
                     123,
                     19,
                     SH1106_COLOR_WHITE);

    sh1106_draw_string(display,
                       8,
                       27,
                       "SSID: " WIFI_STATION_SSID,
                       SH1106_COLOR_WHITE);

    sh1106_draw_string(display,
                       8,
                       43,
                       "Connecting...",
                       SH1106_COLOR_WHITE);

    return sh1106_display(display);
}

/**
 * @brief Draw the assigned Wi-Fi IPv4 address on the OLED.
 */
static esp_err_t app_show_wifi_ip(sh1106_t *display,
                                  const char *ip_address)
{
    ESP_RETURN_ON_FALSE(display != NULL,
                        ESP_ERR_INVALID_ARG,
                        TAG,
                        "Display pointer is NULL");
    ESP_RETURN_ON_FALSE(ip_address != NULL,
                        ESP_ERR_INVALID_ARG,
                        TAG,
                        "IP address pointer is NULL");

    sh1106_clear(display, SH1106_COLOR_BLACK);

    sh1106_draw_rectangle(display,
                          0,
                          0,
                          SH1106_WIDTH,
                          SH1106_HEIGHT,
                          SH1106_COLOR_WHITE);

    sh1106_draw_string(display,
                       11,
                       6,
                       "WIFI CONNECTED",
                       SH1106_COLOR_WHITE);

    sh1106_draw_line(display,
                     4,
                     17,
                     123,
                     17,
                     SH1106_COLOR_WHITE);

    sh1106_draw_string(display,
                       8,
                       23,
                       "SSID: " WIFI_STATION_SSID,
                       SH1106_COLOR_WHITE);

    sh1106_draw_string(display,
                       8,
                       37,
                       "IP ADDRESS:",
                       SH1106_COLOR_WHITE);

    const int16_t ip_width = sh1106_get_string_width(ip_address);
    const int16_t ip_x = (int16_t)((SH1106_WIDTH - ip_width) / 2);

    sh1106_draw_string_no_wrap(display,
                               ip_x,
                               50,
                               ip_address,
                               SH1106_COLOR_WHITE);

    return sh1106_display(display);
}

/**
 * @brief Draw an error screen when the Wi-Fi connection fails.
 */
static esp_err_t app_show_wifi_error(sh1106_t *display)
{
    sh1106_clear(display, SH1106_COLOR_BLACK);

    sh1106_draw_rectangle(display,
                          0,
                          0,
                          SH1106_WIDTH,
                          SH1106_HEIGHT,
                          SH1106_COLOR_WHITE);

    sh1106_draw_string(display,
                       23,
                       10,
                       "WIFI ERROR",
                       SH1106_COLOR_WHITE);

    sh1106_draw_string(display,
                       8,
                       29,
                       "Cannot connect",
                       SH1106_COLOR_WHITE);

    sh1106_draw_string(display,
                       8,
                       43,
                       "SSID: " WIFI_STATION_SSID,
                       SH1106_COLOR_WHITE);

    return sh1106_display(display);
}

void app_main(void)
{
    sh1106_t display = {0};
    char ip_address[WIFI_STATION_IP_STRING_LENGTH] = {0};

    ESP_LOGI(TAG,
             "Initializing I2C: SDA=GPIO%d, SCL/SCK=GPIO%d",
             APP_I2C_SDA_GPIO,
             APP_I2C_SCL_GPIO);

    ESP_ERROR_CHECK(app_i2c_master_init());
    ESP_ERROR_CHECK(sh1106_init(&display,
                                APP_I2C_PORT,
                                APP_SH1106_I2C_ADDRESS));

    ESP_ERROR_CHECK(app_show_wifi_connecting(&display));

    const esp_err_t wifi_result =
        wifi_station_connect(ip_address, sizeof(ip_address));

    bool last_connected = false;
    char last_displayed_ip[WIFI_STATION_IP_STRING_LENGTH] = {0};

    if (wifi_result == ESP_OK) {
        ESP_LOGI(TAG, "OLED will display IPv4 address: %s", ip_address);
        ESP_ERROR_CHECK(app_show_wifi_ip(&display, ip_address));
        snprintf(last_displayed_ip,
                 sizeof(last_displayed_ip),
                 "%s",
                 ip_address);
        last_connected = true;
    } else {
        ESP_LOGE(TAG, "Wi-Fi connection failed: %s",
                 esp_err_to_name(wifi_result));
        ESP_ERROR_CHECK(app_show_wifi_error(&display));
    }

    /*
     * Keep the connection state visible. After a temporary disconnection, the
     * Wi-Fi event handler retries automatically. This loop updates the OLED
     * when the station reconnects or receives a different DHCP address.
     */
    while (true) {
        const bool connected = wifi_station_is_connected();

        if (!connected && last_connected) {
            ESP_LOGW(TAG, "Wi-Fi connection lost; updating OLED status");
            ESP_ERROR_CHECK(app_show_wifi_connecting(&display));
            last_connected = false;
        } else if (connected) {
            const char *current_ip = wifi_station_get_ip_address();

            if (!last_connected ||
                strcmp(last_displayed_ip, current_ip) != 0) {
                ESP_LOGI(TAG, "Updating OLED IPv4 address: %s", current_ip);
                ESP_ERROR_CHECK(app_show_wifi_ip(&display, current_ip));
                snprintf(last_displayed_ip,
                         sizeof(last_displayed_ip),
                         "%s",
                         current_ip);
                last_connected = true;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
