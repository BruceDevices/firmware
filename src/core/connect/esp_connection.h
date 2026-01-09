#ifndef __ESP_CONNECTION_H__
#define __ESP_CONNECTION_H__

#include <esp_now.h>
#include <globals.h>
#include <vector>

#define ESP_FILENAME_SIZE 30
#define ESP_FILEPATH_SIZE 50
#define ESP_DATA_SIZE 150

class EspConnection {
public:
    enum Status {
        CONNECTING,
        STARTED,
        WAITING,
        FAILED,
        SUCCESS,
        ABORTED,
    };

    // Struct has to be 250 B max
    struct Message {
        char filename[ESP_FILENAME_SIZE];
        char filepath[ESP_FILEPATH_SIZE];
        char data[ESP_DATA_SIZE];
        size_t dataSize;
        size_t totalBytes;
        size_t bytesSent;
        bool isFile;
        bool done;
        bool ping;
        bool pong;

        // Constructor to initialize defaults
        Message()
            : dataSize(0), totalBytes(0), bytesSent(0), isFile(false), done(false), ping(false), pong(false) {
        }
    };

    EspConnection();
    ~EspConnection();

    static void setInstance(EspConnection *conn) { instance = conn; }

    static void onDataSentStatic(const wifi_tx_info_t *info, esp_now_send_status_t status);
    static void onDataRecvStatic(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len);

protected:
    Status recvStatus;
    Status sendStatus;
    uint8_t dstAddress[6];
    uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    std::vector<Message> recvQueue;

    bool beginSend();
    bool beginEspnow();

    Message createMessage(String text);
    Message createFileMessage(File file);
    Message createPingMessage();
    Message createPongMessage();

    void sendPing();
    void sendPong(const uint8_t *mac);

    bool setupPeer(const uint8_t *mac);
    void appendPeerToList(const uint8_t *mac);
    void setDstAddress(const uint8_t *address) { memcpy(dstAddress, address, 6); }

    String macToString(const uint8_t *mac);
    void printMessage(Message message);

    void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);

private:
    static EspConnection *instance;
};

#endif
