#if !defined(LITE_VERSION)
#include "serial_commands.h"
#include "core/display.h"
#include "core/mykeyboard.h"

EspSerialCmd::EspSerialCmd() {}

void EspSerialCmd::sendCommands() {
    displayBanner();
    padprintln("Waiting...");

    if (!beginSend()) return;

    sendStatus = CONNECTING;
    Message message;

    delay(100);

    while (1) {
        if (check(EscPress)) {
            displayInfo("Aborting...");
            sendStatus = ABORTED;
            break;
        }

        if (check(SelPress)) { sendStatus = CONNECTING; }

        if (sendStatus == CONNECTING) {
            message = createCmdMessage();

            if (message.dataSize > 0) {
                esp_err_t response = esp_now_send(dstAddress, (uint8_t *)&message, sizeof(message));
                if (response == ESP_OK) sendStatus = SUCCESS;
                else {
                    Serial.printf("Send file response: %s\n", esp_err_to_name(response));
                    sendStatus = FAILED;
                }
            } else {
                Serial.println("No command to send");
                sendStatus = FAILED;
            }
        }

        if (sendStatus == FAILED) {
            displaySentError();
            sendStatus = WAITING;
        }

        if (sendStatus == SUCCESS) {
            displaySentCommand(message.data);
            sendStatus = WAITING;
        }

        delay(100);
    }

    delay(1000);
}

void EspSerialCmd::receiveCommands() {
    displayBanner();
    padprintln("Waiting...");

    recvCommand = "";
    recvQueue.clear();
    recvStatus = CONNECTING;
    Message recvMessage;

    if (!beginEspnow()) return;

    delay(100);

    while (1) {
        if (check(EscPress)) {
            displayInfo("Aborting...");
            recvStatus = ABORTED;
            break;
        }

        if (recvStatus == FAILED) {
            displayRecvError();
            recvStatus = WAITING;
        }
        if (recvStatus == SUCCESS) {
            displayRecvCommand(parseSerialCommand(recvCommand));
            recvStatus = WAITING;
        }

        if (!recvQueue.empty()) {
            recvMessage = recvQueue.front();
            recvQueue.erase(recvQueue.begin());

            recvCommand = recvMessage.data;
            Serial.println(recvCommand);

            if (recvMessage.done) {
                Serial.println("Recv done");
                recvStatus = recvMessage.bytesSent == recvMessage.totalBytes ? SUCCESS : FAILED;
            }
        }

        delay(100);
    }

    delay(1000);
}

EspSerialCmd::Message EspSerialCmd::createCmdMessage() {
    // debounce
    tft.fillScreen(bruceConfig.bgColor);
    delay(500);

    String command = keyboard("", ESP_DATA_SIZE, "Serial Command");
    Message msg = createMessage(command);
    printMessage(msg);

    return msg;
}

void EspSerialCmd::displayBanner() {
    drawMainBorderWithTitle("RECEIVE COMMANDS");
    padprintln("");
}

void EspSerialCmd::displayRecvCommand(bool success) {
    String execution = success ? "Execution success" : "Execution failed";
    Serial.println(execution);

    displayBanner();
    padprintln("Command received: ");
    padprintln(recvCommand);
    padprintln("");
    padprintln(execution);

    displayRecvFooter();
}

void EspSerialCmd::displayRecvError() {
    displayBanner();
    padprintln("Error receiving command");
    displayRecvFooter();
}

void EspSerialCmd::displayRecvFooter() {
    padprintln("\n");
    padprintln("Press [ESC] to leave");
}

void EspSerialCmd::displaySentCommand(const char *command) {
    displayBanner();
    padprintln("Command sent: ");
    padprintln(command);
    displaySentFooter();
}

void EspSerialCmd::displaySentError() {
    displayBanner();
    padprintln("Error sending command");
    displaySentFooter();
}

void EspSerialCmd::displaySentFooter() {
    padprintln("\n");
    padprintln("Press [OK] to send another command");
    padprintln("");
    padprintln("Press [ESC] to leave");
}
#endif
