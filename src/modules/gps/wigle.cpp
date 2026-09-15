/**
 * @file wigle.cpp
 * @author Rennan Cockles - https://github.com/rennancockles
 * @brief Wigle connector
 * @version 0.1
 */

#include "wigle.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include "core/sd_functions.h"
#include "core/wifi/wifi_common.h"

#define CBUFLEN 1024

Wigle::Wigle() {}

Wigle::~Wigle() {}

bool Wigle::_check_token() {
    if (bruceConfig.wigleBasicToken == "") {
        displayError("Wigle token not found", true);
        return false;
    }

    auth_header = "Basic " + bruceConfig.wigleBasicToken;

    if (!wifiConnected) wifiConnectMenu();

    return true;
}

bool Wigle::get_user() {
    if (!_check_token()) return false;

    display_banner();
    padprintln("Connecting to Wigle...");

    WiFiClientSecure client;
    client.setInsecure();
    if (!client.connect(host, 443)) return false;

    client.println("GET /api/v2/profile/user HTTP/1.0");
    client.print("Host: ");
    client.println(host);
    client.println("Connection: close");
    client.println("User-Agent: bruce.wardriving");
    client.print("Authorization: ");
    client.println(auth_header);
    client.println();

    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") break;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, client);

    client.stop();

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return false;
    }

    String userid = doc["userid"];
    wigle_user = userid;

    return true;
}

void Wigle::display_banner() {
    drawMainBorderWithTitle("Wigle Upload");
    padprintln("\n");
}

void Wigle::dump_wigle_info() {
    display_banner();
    padprintln("Logged into Wigle as " + wigle_user);
    padprintln("");
}

void Wigle::send_upload_headers(
    WiFiClientSecure &client, const String &filename, int filesize, const String &boundary
) {
    // Content-Disposition header size.
    int cd_header_len = 147 - 45 - 8 + filename.length() + 2 * (boundary.length() + 2);

    client.println("POST /api/v2/file/upload HTTP/1.0");
    client.print("Host: ");
    client.println(host);
    client.println("Connection: close");
    client.println("User-Agent: bruce.wardriving");
    client.print("Authorization: ");
    client.println(auth_header);
    client.print("Content-Type: multipart/form-data; boundary=");
    client.println(boundary);
    client.print("Content-Length: ");
    client.println(filesize + cd_header_len);
    client.println();

    // Start content-disposition file header:
    client.println("--" + boundary);
    client.print("Content-Disposition: form-data; name=\"file\"; filename=\"");
    client.print(filename);
    client.println("\"");
    client.println("Content-Type: text/csv");
    client.println();
}

bool Wigle::upload(FS *fs, const String &filepath, bool auto_delete) {
    display_banner();

    if (!fs || !get_user()) return false;

    dump_wigle_info();

    File file = fs->open(filepath);
    if (!file) {
        displayError("Failed to open Wigle file", true);
        return false;
    }

    if (!_upload_file(file, "Uploading...")) {
        file.close();
        displayError("File upload error", true);
        return false;
    }

    file.close();
    if (auto_delete) fs->remove(filepath);

    displaySuccess("File upload success", true);
    return true;
}

bool Wigle::upload_all(FS *fs, const String &folder, bool auto_delete) {
    Serial.println("Wigle upload all path: " + folder);

    display_banner();

    File root = fs->open(folder);
    if (!root || !root.isDirectory()) return false;

    if (!fs || !get_user()) return false;

    dump_wigle_info();
    int i = 1;
    bool success;

    while (true) {
        bool isDir;

        success = false;

        String fullPath = root.getNextFileName(&isDir);
        String nameOnly = fullPath.substring(fullPath.lastIndexOf("/") + 1);
        if (fullPath == "") { break; }
        // Serial.printf("Path: %s (isDir: %d)\n", fullPath.c_str(), isDir);

        if (!isDir) {

            int dotIndex = nameOnly.lastIndexOf(".");
            String ext = dotIndex >= 0 ? nameOnly.substring(dotIndex + 1) : "";
            ext.toUpperCase();
            if (ext.equals("CSV")) {
                File file = fs->open(fullPath);

                if (file) {
                    if (!_upload_file(file, "Uploading " + String(i) + "...")) {
                        file.close();
                        displayError("File upload error", true);
                        return false;
                    }
                    i++;
                    success = true;

                    file.close();
                    if (success && auto_delete) fs->remove(fullPath);
                }
            }
        }
    }
    root.close();

    String plural = i > 2 ? "s" : "";
    displaySuccess(String(i - 1) + " file" + plural + " uploaded", true);
    return true;
}

bool Wigle::_upload_file(File file, const String &upload_message) {
    WiFiClientSecure client;
    client.setInsecure();
    if (!client.connect(host, 443)) {
        displayError("Wigle API connection failed", true);
        return false;
    }

    String filename = file.name();
    int filesize = file.size();
    String boundary = "BRUCE";
    boundary.concat(esp_random());

    send_upload_headers(client, filename, filesize, boundary);

    byte cbuf[CBUFLEN];
    float percent = 0;
    while (file.available()) {
        long bytes_available = file.available();
        int toread = CBUFLEN;
        if (bytes_available < CBUFLEN) toread = bytes_available;
        file.read(cbuf, toread);
        client.write(cbuf, toread);
        percent = ((float)file.position() / (float)file.size()) * 100;
        progressHandler(percent, 100, upload_message);
    }

    client.println();
    client.print("--");
    client.print(boundary);
    client.println("--");
    client.println();
    client.flush();

    Serial.println("File transfer complete");

    String serverres = "";

    while (client.connected()) {
        if (client.available()) {
            char c = client.read();
            // Serial.write(c);
            serverres.concat(c);
        }
        if (serverres.length() > 1024) break;
    }

    client.stop();

    return (serverres.indexOf("\"success\":true") > -1);
}
