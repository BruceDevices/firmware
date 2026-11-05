
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <SD.h>

String encryptString(String &plaintext, const String &password_str);

// Streaming helpers for chunked encryption (e.g. web uploads). The XOR keystream is
// position-based, so a file can be encrypted chunk by chunk as long as each chunk knows
// its absolute offset in the plaintext. encryptFileHeader() emits the header once,
// ending with "Data: "; encryptChunkToHex() appends the hex bytes for one chunk.
String encryptFileHeader();
String encryptChunkToHex(const uint8_t *data, size_t len, const String &password_str, size_t streamOffset);

String decryptString(String &cypertext, const String &password_str);

String readDecryptedFile(FS &fs, String filepath);
