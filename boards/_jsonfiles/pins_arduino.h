#include "sdkconfig.h"
#ifdef CONFIG_IDF_TARGET_ESP32
#include "esp32.h"
#elif CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3.h"
#elif CONFIG_IDF_TARGET_ESP32C5
#include "esp32c5.h"
#elif CONFIG_IDF_TARGET_ESP32C6
#include "esp32c6.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3.h"
#elif CONFIG_IDF_TARGET_ESP32P4
#include "esp32p4.h"
#else
#error "Unsupported ESP32 target, need to add a new espxx.h file for this target"
#endif
