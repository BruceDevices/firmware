#include <sdkconfig.h> // CONFIG_IDF_TARGET_ESP32P4 lives here; must precede the P4 guard below
#if !defined(LITE_VERSION) && !defined(CONFIG_IDF_TARGET_ESP32P4)
#include "core/display.h"
#include "core/mykeyboard.h"
#include "serial_commands.h"

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
    hasAuthorizedPeer = false;
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
            if (recvCommand.length() == 0 || recvCommand.length() >= ESP_DATA_SIZE) {
                displayRecvError();
            } else {
                bool authorized = isAuthorizedPeer(lastSenderMac);
                if (!authorized) {
                    String senderStr = macToString(lastSenderMac);
                    bool approveCmd = false;
                    bool trustPeer = false;

                    drawMainBorderWithTitle("CONFIRM COMMAND");
                    padprintln("From: " + senderStr);
                    padprintln("Cmd:  " + recvCommand);
                    padprintln("");

                    std::vector<Option> confirmOpts = {
                        {"Reject",            [&]() { approveCmd = false; }},
                        {"Run once",          [&]() { approveCmd = true; } },
                        {"Always allow peer", [&]() {
                             approveCmd = true;
                             trustPeer = true;
                         }    },
                    };
                    loopOptions(confirmOpts);

                    if (trustPeer) { setDstAddress(lastSenderMac); }
                    authorized = approveCmd;
                }

                if (authorized) {
                    displayRecvCommand(parseSerialCommand(recvCommand));
                } else {
                    displayBanner();
                    padprintln("Command rejected");
                    displayRecvFooter();
                }
            }
            recvStatus = WAITING;
        }

        if (!recvQueue.empty()) {
            recvMessage = recvQueue.front();
            recvQueue.erase(recvQueue.begin());

            size_t safeLen = std::min<size_t>(recvMessage.dataSize, ESP_DATA_SIZE - 1);
            recvCommand = String(recvMessage.data).substring(0, safeLen);
            recvCommand.trim();
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
    if (command == "\x1B") command = "";
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
#elif !defined(LITE_VERSION)
// ESP-NOW command sharing is unavailable on ESP32-P4 (no esp_now_* API). Stub the entry
// points so the menus keep working and just tell the user it is unsupported here.
#include "core/display.h"
#include "serial_commands.h"

EspSerialCmd::EspSerialCmd() {}

void EspSerialCmd::sendCommands() { displayWarning("Not available on this device", true); }
void EspSerialCmd::receiveCommands() { displayWarning("Not available on this device", true); }
#endif
