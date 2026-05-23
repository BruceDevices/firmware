// TODO: Be able to read bytes from server in background/task
//       so there is no loss of data when inputing
#ifndef LITE_VERSION
#include "modules/wifi/tcp_utils.h"
#include "core/wifi/wifi_common.h"
#include "core/display.h"

bool inputMode;

void listenTcpPort() {
    if (!wifiConnected) wifiConnectMenu();

    String portNumber = num_keyboard("", 5, "TCP port to listen");
    if (portNumber.length() == 0) {
        displayError("No port number given, exiting");
        return;
    }
    int portNumberInt = atoi(portNumber.c_str());
    if (portNumberInt == 0) {
        displayError("Invalid port number, exiting");
        return;
    }

    WiFiClient tcpClient;
    drawMainBorderWithTitle("LISTEN TCP");
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    WiFiServer server(portNumberInt);
    server.begin();

    tft.setCursor(10, BORDER_PAD_Y + FM * LH);
    tft.println("Listening on:");
    tft.setCursor(10, tft.getCursorY());
    tft.print(WiFi.localIP().toString().c_str());
    tft.println(":" + portNumber);

    for (;;) {
        WiFiClient client = server.accept();

        if (client) {
            Serial.println("Client connected");
            tft.setCursor(10, tft.getCursorY());
            tft.println("Client connected");

            while (client.connected()) {
                if (inputMode) {
                    String keyString = keyboard("", 16, "send input data, q=quit");
                    if (keyString == "q") {
                        displayError("Exiting Listener");
                        client.stop();
                        server.stop();
                        return;
                    }
                    inputMode = false;
                    drawMainBorderWithTitle("LISTEN TCP");
                    tft.setTextSize(FP);
                    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                    tft.setCursor(10, BORDER_PAD_Y + FM * LH);
                    if (keyString.length() > 0) {
                        client.print(keyString);
                        Serial.print(keyString);
                    }
                } else {
                    if (client.available()) {
                        char incomingChar = client.read();
                        tft.print(incomingChar);
                        Serial.print(incomingChar);
                    }
                    if (check(SelPress)) { inputMode = true; }
                }
            }
            client.stop();
            Serial.println("Client disconnected");
            displayError("Client disconnected");
        }
        if (check(EscPress)) {
            displayError("Exiting Listener");
            server.stop();
            break;
        }
    }
}

void clientTCP() {
    if (!wifiConnected) wifiConnectMenu();

    String serverIP = keyboard("", 15, "Enter server IP");
    String portString = num_keyboard("", 5, "Enter server Port");
    int portNumber = atoi(portString.c_str());

    if (serverIP.length() == 0 || portNumber == 0) {
        displayError("Invalid IP or Port");
        return;
    }

    WiFiClient client;
    drawMainBorderWithTitle("TCP CLIENT");
    tft.setTextSize(FP);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);

    tft.setCursor(10, BORDER_PAD_Y + FM * LH);
    tft.println("Connecting to:");
    tft.setCursor(10, tft.getCursorY());
    tft.println(serverIP + ":" + portString);

    if (!client.connect(serverIP.c_str(), portNumber)) {
        displayError("Connection failed");
        return;
    }

    tft.setCursor(10, tft.getCursorY());
    tft.println("Connected!");
    Serial.println("Connected to server");

    while (client.connected()) {
        if (inputMode) {
            String keyString = keyboard("", 16, "send input data");
            inputMode = false;
            drawMainBorderWithTitle("TCP CLIENT");
            tft.setTextSize(FP);
            tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
            tft.setCursor(10, BORDER_PAD_Y + FM * LH);
            if (keyString.length() > 0) {
                client.print(keyString);
                Serial.print(keyString);
            }
        } else {
            if (client.available()) {
                char incomingChar = client.read();
                tft.print(incomingChar);
                Serial.print(incomingChar);
            }
            if (check(SelPress)) { inputMode = true; }
        }
        if (check(EscPress)) {
            displayError("Exiting Client");
            client.stop();
            break;
        }
    }

    displayError("Connection closed.");
    Serial.println("Connection closed.");
    client.stop();
}
#endif
