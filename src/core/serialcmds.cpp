#include "serialcmds.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "utils.h"
#include <globals.h>

QueueHandle_t cmdQueue = nullptr;
QueueHandle_t rspQueue = nullptr;
TaskHandle_t serialcmdsTaskHandle;

struct CmdPacket {
    char text[512]; // command size
};
bool parseSerialCommand(const String &command, bool waitForResponse) {
    if (!cmdQueue || !rspQueue) {
        Serial.println("Command or response queue not initialized");
        return false;
    }
    CmdPacket packet;
    memset(&packet, 0, sizeof(packet));
    strncpy(packet.text, command.c_str(), sizeof(packet.text) - 1);

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

static inline bool isQueryOnlyCommand(const String &cmd) {
    return cmd.startsWith("nav") || cmd.startsWith("option") || cmd.startsWith("display") ||
           cmd.startsWith("info") || cmd.startsWith("uptime") || cmd.startsWith("free") ||
           cmd.startsWith("storage") || cmd.startsWith("help") || cmd.startsWith("date") ||
           cmd.startsWith("i2c");
}

void handleSerialCommands(SerialCli &serialCli) {
    CmdPacket packet;
    if (cmdQueue && rspQueue) {
        if (xQueueReceive(cmdQueue, &packet, 0) == pdTRUE) {
            bool result = serialCli.parse(String(packet.text));
            xQueueSend(rspQueue, &result, 0);
            Serial.println("COMMAND: " + String(packet.text));
            Serial.printf("[CLI] Result: %s\n", result ? "TRUE" : "FALSE");
        }
    }
    static String inputBuffer = "";
    static unsigned long lastInputTime = 0;

    while (serialDevice->available()) {
        int c = serialDevice->read();
        if (c < 0) break;
        if (c == '\r' || c == '\n') {
            if (inputBuffer.length() > 0) {
                String cmd = inputBuffer;
                inputBuffer = "";
                lastInputTime = 0;
                cmd.trim();
                if (cmd.length() > 0) {
                    Serial.println("COMMAND: " + cmd);
                    serialCli.parse(cmd);
                    if (!isQueryOnlyCommand(cmd)) {
                        backToMenu();
                    }
                }
            }
            serialDevice->print("# ");
        } else if (c == '\b' || c == 127) {
            if (inputBuffer.length() > 0) {
                inputBuffer.remove(inputBuffer.length() - 1);
            }
            lastInputTime = millis();
        } else if (c >= 32 && c <= 126) {
            inputBuffer += (char)c;
            lastInputTime = millis();
        }
    }

    // Idle line flush timeout: if characters were buffered but no newline arrived
    // (e.g. programmatic WebSerial clients like https://bruce.computer/my_bruce),
    // automatically execute after 150ms of silence.
    if (inputBuffer.length() > 0 && lastInputTime > 0 && (millis() - lastInputTime >= 150)) {
        String cmd = inputBuffer;
        inputBuffer = "";
        lastInputTime = 0;
        cmd.trim();
        if (cmd.length() > 0) {
            Serial.println("COMMAND: " + cmd);
            serialCli.parse(cmd);
            if (!isQueryOnlyCommand(cmd)) {
                backToMenu();
            }
        }
        serialDevice->print("# ");
    }
}

void _serialCmdsTaskLoop(void *pvParameters) {
    Serial.begin(115200);
    vTaskDelay(pdMS_TO_TICKS(100));
    serialDevice->println("\r\n==========================================");
    serialDevice->println("   Bruce ESP32-S3-Zero CLI Ready");
    serialDevice->println("   Type 'help' or 'info' for commands");
    serialDevice->println("==========================================");
    serialDevice->print("# ");

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
