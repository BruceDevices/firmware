#include <stdio.h>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/queue.h"

#define PMU_INPUT_PIN                   (gpio_num_t)CONFIG_PMU_INTERRUPT_PIN    /*!< axp power chip interrupt Pin*/
#define PMU_INPUT_PIN_SEL               (1ULL<<PMU_INPUT_PIN)


/*
! WARN:
Please do not run the example without knowing the external load voltage of the PMU,
it may burn your external load, please check the voltage setting before running the example,
if there is any loss, please bear it by yourself
*/
#ifndef XPOWERS_NO_ERROR
#error "Running this example is known to not damage the device! Please go and uncomment this!"
#endif

static const char *TAG = "mian";

extern esp_err_t pmu_init();
extern esp_err_t i2c_init(void);
extern void pmu_isr_handler();

static void pmu_handler_task(void *);
static QueueHandle_t  gpio_evt_queue = NULL;

static void IRAM_ATTR pmu_irq_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void irq_init()
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = PMU_INPUT_PIN_SEL;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    gpio_set_intr_type(PMU_INPUT_PIN, GPIO_INTR_NEGEDGE);
    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(PMU_INPUT_PIN, pmu_irq_handler, (void *) PMU_INPUT_PIN);

}

extern "C" void app_main(void)
{
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(5, sizeof(uint32_t));

    // Register PMU interrupt pins
    irq_init();

#if CONFIG_I2C_COMMUNICATION_METHOD_CALLBACK_RW || \
    ((ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)) && defined(CONFIG_XPOWERS_ESP_IDF_NEW_API))
    ESP_ERROR_CHECK(i2c_init());
    ESP_LOGI(TAG, "I2C initialized successfully");
#endif

    ESP_ERROR_CHECK(pmu_init());

    xTaskCreate(pmu_handler_task, "App/pwr", 4 * 1024, NULL, 10, NULL);

    ESP_LOGI(TAG, "Run...");

}


static void pmu_handler_task(void *args)
{
    uint32_t io_num;
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            pmu_isr_handler();
        }
    }
}



