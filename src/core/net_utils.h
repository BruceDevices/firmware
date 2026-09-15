#include <HTTPClient.h>
#include <WiFi.h>

bool internetConnection();

String getManufacturer(const String &mac);

String MAC(uint8_t *data);

void stringToMAC(const std::string &macStr, uint8_t MAC[6]);

// Função para converter IP para string
String ipToString(const uint8_t *ip);

// Função para converter MAC para string
String macToString(const uint8_t *mac);
