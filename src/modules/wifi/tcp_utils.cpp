// TODO: Be able to read bytes from server in background/task
//       so there is no loss of data when inputing
#include "modules/wifi/tcp_utils.h"
#include "core/wifi/wifi_common.h"

bool inputMode;

void listenTcpPort() {
    if (!wifiConnected) wifiConnectMenu();

    WiFiClient tcpClient;
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

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

    WiFiServer server(portNumberInt);
    server.begin();

    tft.println("Listening...");
    tft.print(WiFi.localIP().toString().c_str());
    tft.println(":" + portNumber);

    for (;;) {
        WiFiClient client = server.accept(); // Wait for a client to connect

        if (client) {
            Serial.println("Client connected");
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
                    tft.fillScreen(TFT_BLACK);
                    tft.setCursor(0, 0);
                    if (keyString.length() > 0) {
                        client.print(keyString); // Send the entire string to the client
                        Serial.print(keyString);
                    }
                } else {
                    if (client.available()) {
                        char incomingChar = client.read(); // Read one byte at time from the client
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
    if (!client.connect(serverIP.c_str(), portNumber)) {
        displayError("Connection failed");
        return;
    }

    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.println("Connected to:");
    tft.println(serverIP + ":" + portString);
    Serial.println("Connected to server");

    while (client.connected()) {
        if (inputMode) {
            String keyString = keyboard("", 16, "send input data");
            inputMode = false;
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(0, 0);
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
