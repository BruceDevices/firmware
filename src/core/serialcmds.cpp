#include "serialcmds.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "utils.h"
#include <globals.h>
#include <cstdio>

QueueHandle_t cmdQueue = nullptr;
QueueHandle_t rspQueue = nullptr;
TaskHandle_t serialcmdsTaskHandle;

struct CmdPacket {
    char text[512]; // command size
    uint32_t webRequestId;
};

static SemaphoreHandle_t webCommandMutex = nullptr;
static uint32_t webCommandCounter = 0;
static uint32_t webCommandId = 0;
static bool webCommandDone = false;
static bool webCommandSuccess = false;
static char webCommandOutput[2048] = {0};

// A temporary SerialDevice tee captures only writes from the command task.
// All output is still forwarded to the active USB or BLE serial device.
class CaptureSerialDevice : public SerialDevice {
public:
    explicit CaptureSerialDevice(SerialDevice *device)
        : device_(device), owner_(xTaskGetCurrentTaskHandle()) {}

    size_t println(const String &s) override {
        size_t written = device_->println(s);
        append(s.c_str(), s.length());
        append("\r\n", 2);
        return written;
    }
    size_t println(size_t n) override {
        size_t written = device_->println(n);
        String value((unsigned long)n);
        append(value.c_str(), value.length());
        append("\r\n", 2);
        return written;
    }
    size_t println(uint32_t n) override {
        size_t written = device_->println(n);
        String value(n);
        append(value.c_str(), value.length());
        append("\r\n", 2);
        return written;
    }
    size_t println() override {
        size_t written = device_->println();
        append("\r\n", 2);
        return written;
    }
    size_t println(int n, int format = DEC) override {
        size_t written = device_->println(n, format);
        String value(n, format);
        append(value.c_str(), value.length());
        append("\r\n", 2);
        return written;
    }
    size_t print(int n, int format = DEC) override {
        size_t written = device_->print(n, format);
        String value(n, format);
        append(value.c_str(), value.length());
        return written;
    }
    size_t print(const String &s) override {
        size_t written = device_->print(s);
        append(s.c_str(), s.length());
        return written;
    }
    void vprintf(const char *fmt, va_list args) override {
        char formatted[512];
        va_list copy;
        va_copy(copy, args);
        int length = vsnprintf(formatted, sizeof(formatted), fmt, copy);
        va_end(copy);
        if (length > 0) append(formatted, (size_t)length < sizeof(formatted) ? (size_t)length : sizeof(formatted) - 1);
        device_->vprintf(fmt, args);
    }
    int read() override { return device_->read(); }
    size_t write(uint8_t *str, size_t size) override {
        append((const char *)str, size);
        return device_->write(str, size);
    }
    void flush() override { device_->flush(); }
    int available() override { return device_->available(); }
    String readStringUntil(char terminator) override { return device_->readStringUntil(terminator); }
    String output() const { return String(output_); }

private:
    SerialDevice *device_;
    TaskHandle_t owner_;
    char output_[2048] = {0};
    size_t length_ = 0;

    void append(const char *data, size_t length) {
        if (xTaskGetCurrentTaskHandle() != owner_ || !data || !length) return;
        size_t available = sizeof(output_) - length_ - 1;
        size_t copied = length < available ? length : available;
        if (copied) {
            memcpy(output_ + length_, data, copied);
            length_ += copied;
            output_[length_] = '\0';
        }
    }
};

static void completeWebCommand(uint32_t id, const String &output, bool success) {
    if (!webCommandMutex) return;
    if (xSemaphoreTake(webCommandMutex, portMAX_DELAY) == pdTRUE) {
        if (id == webCommandId) {
            size_t length = output.length();
            if (length >= sizeof(webCommandOutput)) length = sizeof(webCommandOutput) - 1;
            memcpy(webCommandOutput, output.c_str(), length);
            webCommandOutput[length] = '\0';
            webCommandSuccess = success;
            webCommandDone = true;
        }
        xSemaphoreGive(webCommandMutex);
    }
}

uint32_t queueWebSerialCommand(const String &command) {
    if (!cmdQueue || !rspQueue) return 0;
    if (!webCommandMutex) webCommandMutex = xSemaphoreCreateMutex();
    if (!webCommandMutex || xSemaphoreTake(webCommandMutex, portMAX_DELAY) != pdTRUE) return 0;

    if (webCommandId && !webCommandDone) {
        xSemaphoreGive(webCommandMutex);
        return 0;
    }

    CmdPacket packet = {};
    strncpy(packet.text, command.c_str(), sizeof(packet.text) - 1);
    packet.webRequestId = ++webCommandCounter;
    if (!packet.webRequestId) packet.webRequestId = ++webCommandCounter;

    webCommandId = packet.webRequestId;
    webCommandDone = false;
    webCommandSuccess = false;
    webCommandOutput[0] = '\0';

    if (xQueueSend(cmdQueue, &packet, 0) != pdTRUE) {
        webCommandId = 0;
        webCommandDone = true;
        xSemaphoreGive(webCommandMutex);
        return 0;
    }

    uint32_t id = webCommandId;
    xSemaphoreGive(webCommandMutex);
    return id;
}

bool getWebSerialCommandResult(uint32_t id, String &output, bool &done, bool &success) {
    if (!webCommandMutex || xSemaphoreTake(webCommandMutex, portMAX_DELAY) != pdTRUE) return false;
    bool found = id != 0 && id == webCommandId;
    if (found) {
        output = String(webCommandOutput);
        done = webCommandDone;
        success = webCommandSuccess;
    }
    xSemaphoreGive(webCommandMutex);
    return found;
}
bool parseSerialCommand(const String &command, bool waitForResponse) {
    if (!cmdQueue || !rspQueue) {
        Serial.println("Command or response queue not initialized");
        return false;
    }
    CmdPacket packet = {};
    strncpy(packet.text, command.c_str(), sizeof(packet.text) - 1);
    packet.webRequestId = 0;

    // Enqueue the command packet for processing
    if (xQueueSend(cmdQueue, &packet, 0) != pdTRUE) {
        Serial.println("Failed to send command to queue");
        return false;
    }
    if (!waitForResponse) { return true; }
    // Wait for the response
    bool result;
    if (xQueueReceive(rspQueue, &result, pdMS_TO_TICKS(20))) { return result; }
    Serial.println("Failed to receive command response");
    return false;
}

void handleSerialCommands(SerialCli &serialCli) {
    CmdPacket packet;
    if (cmdQueue && rspQueue) {
        if (xQueueReceive(cmdQueue, &packet, 0) == pdTRUE) {
            bool result;
            if (packet.webRequestId) {
                SerialDevice *originalDevice = serialDevice;
                CaptureSerialDevice capture(originalDevice);
                serialDevice = &capture;
                result = serialCli.parse(String(packet.text));
                // Preserve a device switch deliberately performed by the command.
                if (serialDevice == &capture) serialDevice = originalDevice;
                completeWebCommand(packet.webRequestId, capture.output(), result);
            } else {
                result = serialCli.parse(String(packet.text));
                xQueueSend(rspQueue, &result, 0);
            }
            Serial.println("COMMAND: " + String(packet.text));
            Serial.printf("[CLI] Result: %s\n", result ? "TRUE" : "FALSE");
        }
    }
    if (!serialDevice->available()) return;

    String cmd_str = serialDevice->readStringUntil('\n');
    Serial.println("COMMAND: " + cmd_str);
    serialCli.parse(cmd_str);
    serialDevice->print("# "); // prompt

    // forced menu redrawn if the command is not "nav" or "option"
    // it allows navigation commands to be executed without returning to the menu, while other commands will
    // return to the menu after execution.
    String cmd_trimmed = cmd_str;
    cmd_trimmed.trim();
    if (!cmd_trimmed.startsWith("nav") && !cmd_trimmed.startsWith("option")) { backToMenu(); }
}

void _serialCmdsTaskLoop(void *pvParameters) {
    Serial.begin(115200);
    while (1) {
        handleSerialCommands(serialCli);
        // handleSerialCommands() runs one command per pass, so a burst queued by
        // the BLE app used to pay the full idle tick between each one. Still always
        // yields, just sooner when there is more input waiting.
        vTaskDelay(pdMS_TO_TICKS(serialDevice->available() ? 1 : 10));
    }
}

void startSerialCommandsHandlerTask(bool initQueues) {
    if (initQueues) {
        cmdQueue = xQueueCreate(2, sizeof(CmdPacket));
        rspQueue = xQueueCreate(2, sizeof(bool));
        if (!webCommandMutex) webCommandMutex = xSemaphoreCreateMutex();
    }

    xTaskCreatePinnedToCore(
        _serialCmdsTaskLoop,         // Function to implement the task
        "serialcmds",                // Name of the task (any string)
        SERIAL_CMDS_TASK_STACK_SIZE, // Stack size in bytes
        NULL, // This is a pointer to the parameter that will be passed to the new task. We are not using it
              // here and therefore it is set to NULL.
        2,    // Priority of the task
        &serialcmdsTaskHandle, // Task handle (optional, can be NULL).
#if SOC_CPU_CORES_NUM > 1
        1 // Core where the task should run. By default, all your Arduino code runs on Core 1 and the Wi-Fi
          // and RF functions
#else
        0 // Core where the task should run. ESP32-C5 has only one core
#endif
    ); // (these are usually hidden from the Arduino environment) use the Core 0.
    if (!serialcmdsTaskHandle) { Serial.println("Failed to create Serial Commands Handler task"); }
}
