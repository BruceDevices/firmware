#include <stdio.h>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_idf_version.h"
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_XPOWERS_ESP_IDF_NEW_API)
#include "driver/i2c_master.h"
#else
#include "driver/i2c.h"
#endif

static const char *TAG = "XPowersLib";

#define I2C_MASTER_NUM                  (i2c_port_t)CONFIG_I2C_MASTER_PORT_NUM
#define I2C_MASTER_SDA_IO               (gpio_num_t)CONFIG_PMU_I2C_SDA
#define I2C_MASTER_SCL_IO               (gpio_num_t)CONFIG_PMU_I2C_SCL
#define I2C_MASTER_FREQ_HZ              CONFIG_I2C_MASTER_FREQUENCY /*!< I2C master clock frequency */

#if CONFIG_I2C_COMMUNICATION_METHOD_CALLBACK_RW

#define I2C_MASTER_TX_BUF_DISABLE       0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE       0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS           1000

#define WRITE_BIT                       I2C_MASTER_WRITE            /*!< I2C master write */
#define READ_BIT                        I2C_MASTER_READ             /*!< I2C master read */
#define ACK_CHECK_EN                    0x1                         /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                   0x0                         /*!< I2C master will not check ack from slave */
#define ACK_VAL                         (i2c_ack_type_t)0x0         /*!< I2C ack value */
#define NACK_VAL                        (i2c_ack_type_t)0x1         /*!< I2C nack value */


#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_XPOWERS_ESP_IDF_NEW_API)
// The new API passes the device address before initialization,
// AXP2102, 1XP192 belong to the same address
#define PMU_SLAVE_ADDRESS   0X34
i2c_master_dev_handle_t  i2c_device;
#endif

/**
 * @brief Read a sequence of bytes from a pmu registers
 */
int pmu_register_read(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len)
{
    esp_err_t ret;
    if (len == 0) {
        return ESP_OK;
    }
    if (data == NULL) {
        return ESP_FAIL;
    }

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_XPOWERS_ESP_IDF_NEW_API)

    ret = i2c_master_transmit_receive(
              i2c_device,
              (const uint8_t *)&regAddr,
              1,
              data,
              len,
              -1);
#else

    i2c_cmd_handle_t cmd;

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devAddr << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, regAddr, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret =  i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdTICKS_TO_MS(1000));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PMU i2c_master_cmd_begin FAILED! > ");
        return ESP_FAIL;
    }
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devAddr << 1) | READ_BIT, ACK_CHECK_EN);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, &data[len - 1], NACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdTICKS_TO_MS(1000));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PMU READ FAILED! > ");
    }
#endif
    return ret == ESP_OK ? 0 : -1;
}

/**
 * @brief Write a byte to a pmu register
 */
int pmu_register_write_byte(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len)
{
    esp_err_t ret;
    if (data == NULL) {
        return ESP_FAIL;
    }

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_XPOWERS_ESP_IDF_NEW_API)
    uint8_t *write_buffer = (uint8_t *)malloc(sizeof(uint8_t) * (len + 1));
    if (!write_buffer) {
        return -1;
    }
    write_buffer[0] = regAddr;
    memcpy(write_buffer + 1, data, len);

    ret = i2c_master_transmit(
              i2c_device,
              write_buffer,
              len + 1,
              -1);
    free(write_buffer);
#else
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devAddr << 1) |  I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, regAddr, ACK_CHECK_EN);
    i2c_master_write(cmd, data, len, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdTICKS_TO_MS(1000));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PMU WRITE FAILED! < ");
    }
#endif
    return ret == ESP_OK ? 0 : -1;
}


/**
 * @brief i2c master initialization
 */
esp_err_t i2c_init(void)
{
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_XPOWERS_ESP_IDF_NEW_API)

    ESP_LOGI(TAG, "Implemented using read and write callback methods (Use higher version >= 5.0 API)");

    i2c_master_bus_handle_t  bus_handle;
    i2c_master_bus_config_t i2c_bus_config;
    memset(&i2c_bus_config, 0, sizeof(i2c_bus_config));
    i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_bus_config.i2c_port = I2C_MASTER_NUM;
    i2c_bus_config.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
    i2c_bus_config.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
    i2c_bus_config.glitch_ignore_cnt = 7;

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));

    i2c_device_config_t i2c_dev_conf = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PMU_SLAVE_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    return i2c_master_bus_add_device(bus_handle, &i2c_dev_conf, &i2c_device);

#else

    ESP_LOGI(TAG, "Implemented using read and write callback methods (Use lower version < 5.0 API)");

    i2c_config_t i2c_conf ;
    memset(&i2c_conf, 0, sizeof(i2c_conf));
    i2c_conf.mode = I2C_MODE_MASTER;
    i2c_conf.sda_io_num = I2C_MASTER_SDA_IO;
    i2c_conf.scl_io_num = I2C_MASTER_SCL_IO;
    i2c_conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(I2C_MASTER_NUM, &i2c_conf);
    return i2c_driver_install(I2C_MASTER_NUM, i2c_conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
#endif
}


#else


#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_XPOWERS_ESP_IDF_NEW_API)

#include "soc/clk_tree_defs.h"
i2c_master_bus_handle_t bus_handle;

// * Using the new API of esp-idf 5.x, you need to pass the I2C BUS handle,
// * which is useful when the bus shares multiple devices.

esp_err_t i2c_init(void)
{
    i2c_master_bus_config_t i2c_bus_config;
    memset(&i2c_bus_config, 0, sizeof(i2c_bus_config));
    i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_bus_config.i2c_port = I2C_MASTER_NUM;
    i2c_bus_config.scl_io_num = I2C_MASTER_SCL_IO;
    i2c_bus_config.sda_io_num = I2C_MASTER_SDA_IO;
    i2c_bus_config.glitch_ignore_cnt = 7;
    return i2c_new_master_bus(&i2c_bus_config, &bus_handle);
}

#endif //ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)


#endif //CONFIG_I2C_COMMUNICATION_METHOD_CALLBACK_RW
