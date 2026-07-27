#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_demos.h"
#include "mochi_faces.h"
#include "sh1106.h"

/*
 * I2C pins requested for this project.
 * Some OLED modules label the I2C clock pin as SCK instead of SCL.
 */
#define APP_I2C_SDA_GPIO              23
#define APP_I2C_SCL_GPIO              22
#define APP_I2C_PORT                  I2C_NUM_0
#define APP_I2C_GLITCH_IGNORE_COUNT    7
#define APP_SH1106_I2C_ADDRESS      0x3C

static const char *TAG = "main";

/** Initialize the ESP-IDF I2C master bus. */
static esp_err_t app_i2c_master_init(i2c_master_bus_handle_t *bus_handle)
{
    ESP_RETURN_ON_FALSE(bus_handle != NULL,
                        ESP_ERR_INVALID_ARG,
                        TAG,
                        "Bus handle output pointer is NULL");

    const i2c_master_bus_config_t bus_config = {
        .i2c_port = APP_I2C_PORT,
        .sda_io_num = APP_I2C_SDA_GPIO,
        .scl_io_num = APP_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = APP_I2C_GLITCH_IGNORE_COUNT,
        .flags.enable_internal_pullup = true,
    };

    return i2c_new_master_bus(&bus_config, bus_handle);
}

void app_main(void)
{
    i2c_master_bus_handle_t i2c_bus = NULL;
    sh1106_t display = {0};

    ESP_LOGI(TAG,
             "Initializing I2C: SDA=GPIO%d, SCL/SCK=GPIO%d",
             APP_I2C_SDA_GPIO,
             APP_I2C_SCL_GPIO);

    ESP_ERROR_CHECK(app_i2c_master_init(&i2c_bus));
    ESP_ERROR_CHECK(sh1106_init(&display,
                                i2c_bus,
                                APP_SH1106_I2C_ADDRESS));

    while (true) {
        /*
         * Show all 15 mochi-face expressions.
         * Each expression is animated for approximately 1.5 seconds.
         */
        ESP_ERROR_CHECK(mochi_face_demo_all(&display, 1500, 1));

        /*
         * Direct single-expression example:
         *
         * ESP_ERROR_CHECK(
         *     mochi_face_show(&display, MOCHI_FACE_HAPPY, 0));
         */

        sh1106_clear(&display, SH1106_COLOR_BLACK);
        ESP_ERROR_CHECK(sh1106_display(&display));
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
