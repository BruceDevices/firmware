#include "storage_commands.h"
#include "core/sd_functions.h"
#include "helpers.h"
#include <globals.h>

// YModem logging - set to 1 to enable detailed transfer logging
// #define YMODEM_LOGGING_ENABLED 1

uint32_t listCallback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument("filepath");
    String filepath = arg.getValue();
    filepath.trim();

    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    FS *fs;
    if (!getFsStorage(fs) || !(*fs).exists(filepath)) return false;

    File root = fs->open(filepath);

    while (true) {
        bool isDir;
        String fullPath = root.getNextFileName(&isDir);
        String nameOnly = fullPath.substring(fullPath.lastIndexOf("/") + 1);
        if (fullPath == "") { break; }
        // Serial.printf("Path: %s (isDir: %d)\n", fullPath.c_str(), isDir);

        serialDevice->print(nameOnly);
        if (esp_get_free_heap_size() > (String("Fo:" + nameOnly + ":0\n").length()) + 1024) {
            if (isDir) {
                // Serial.printf("Directory: %s\n", fullPath.c_str());
                serialDevice->println("\t<DIR>");
            } else {
                // For files, we need to get the size, so we open the file briefly
                // Serial.printf("Opening file for size check: %s\n", fullPath.c_str());
                File file = fs->open(fullPath);
                // Serial.printf("File size: %llu bytes\n", file.size());
                if (file) {
                    serialDevice->print("\t");
                    serialDevice->println(file.size());
                    // serialDevice->println(file.path());
                    // serialDevice->println(file.getLastWrite());  // TODO: parse to localtime
                    file.close();
                }
            }
        } else break;
    }
    root.close();

    return true;
}

uint32_t readCallback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument("filepath");
    String filepath = arg.getValue();
    filepath.trim();

    if (filepath.length() == 0) return false;

    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    FS *fs;
    if (!getFsStorage(fs) || !(*fs).exists(filepath)) return false;

    serialDevice->println(readSmallFile(*fs, filepath));
    return true;
}

uint32_t md5Callback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument("filepath");
    String filepath = arg.getValue();
    filepath.trim();

    if (filepath.length() == 0) return false;

    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    FS *fs;
    if (!getFsStorage(fs) || !(*fs).exists(filepath)) return false;

    serialDevice->println(md5File(*fs, filepath));
    return true;
}

uint32_t crc32Callback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument("filepath");
    String filepath = arg.getValue();
    filepath.trim();

    if (filepath.length() == 0) return false;

    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    FS *fs;
    if (!getFsStorage(fs) || !(*fs).exists(filepath)) return false;

    serialDevice->println(crc32File(*fs, filepath));
    return true;
}

uint32_t removeCallback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument("filepath");
    String filepath = arg.getValue();
    filepath.trim();

    if (filepath.length() == 0) return false;

    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    FS *fs;
    if (!getFsStorage(fs)) return false;
    if (!(*fs).exists(filepath)) {
        serialDevice->println("File does not exist");
        return false;
    }

    if ((*fs).remove(filepath)) {
        serialDevice->println("File removed");
        return true;
    }

    serialDevice->println("Error removing file");
    return false;
}
#ifndef LITE_VERSION
uint32_t writeCallback(cmd *c) {
    Command cmd(c);

    Argument filepathArg = cmd.getArgument("filepath");
    Argument sizeArg = cmd.getArgument("size");
    String filepath = filepathArg.getValue();
    String sizeStr = sizeArg.getValue();
    filepath.trim();
    int fileSize = sizeStr.toInt();

    if (filepath.length() == 0) return false;

    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    if (fileSize < SAFE_STACK_BUFFER_SIZE) fileSize = SAFE_STACK_BUFFER_SIZE;

    FS *fs;
    if (!getFsStorage(fs)) return false;

    char *txt = _readFileFromSerial(fileSize + 2);
    if (strlen(txt) == 0) return false;

    File f = fs->open(filepath, FILE_WRITE, true);
    if (!f) return false;

    f.write((const uint8_t *)txt, strlen(txt));
    f.close();
    free(txt);

    serialDevice->println("File written: " + filepath);
    return true;
}

void writeByte(uint8_t byte) { serialDevice->write(&byte, 1); }

// Helper function to log YModem events to file
void logYmodem(const String &message) {
    FS *fs;
    if (!getFsStorage(fs)) return;

    File logFile = fs->open("/ymodem.log", FILE_APPEND);
    if (logFile) {
        logFile.println(String(millis()) + ": " + message);
        logFile.close();
    }
}

// Y-modem protocol constants
#define SOH 0x01 // Start of Header (128 byte blocks)
#define STX 0x02 // Start of Header (1024 byte blocks)
#define EOT 0x04 // End of Transmission
#define ACK 0x06 // Acknowledge
#define NAK 0x15 // Not Acknowledge
#define CAN 0x18 // Cancel
#define CRC 0x43 // 'C' - CRC mode request

// Simple CRC16 calculation for Y-modem
uint16_t crc16_update(uint16_t crc, uint8_t data) {
    crc ^= (uint16_t)data << 8;
    for (int i = 0; i < 8; i++) {
        if (crc & 0x8000) {
            crc = (crc << 1) ^ 0x1021;
        } else {
            crc <<= 1;
        }
    }
    return crc;
}

uint16_t crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0x0000; // Y-modem uses initial value 0x0000 for CRC-16-CCITT
    for (size_t i = 0; i < length; i++) { crc = crc16_update(crc, data[i]); }
    return crc;
}

// Helper function to remove YModem padding (0x1A) from end of data
size_t removePadding(uint8_t *data, size_t length) {
    // Remove trailing 0x1A (EOF/padding) bytes
    while (length > 0 && data[length - 1] == 0x1A) { length--; }
    return length;
}

uint32_t ymodemReceiveCallback(cmd *c) {
    Command cmd(c);
    Argument arg = cmd.getArgument("filepath");
    String filepath = arg.getValue();
    filepath.trim();

    if (filepath.length() == 0) { return false; }
    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    FS *fs;
    if (!getFsStorage(fs)) { return false; }

#ifdef YMODEM_LOGGING_ENABLED
    logYmodem("YModem: Starting receive to " + filepath.substring(filepath.lastIndexOf("/") + 1));
#endif

    // YModem communication goes to serialDevice (UART)
    serialDevice->println("Starting Y-modem receive to: " + filepath);
    serialDevice->println("Send file using Y-modem protocol (128-byte blocks only)");
    serialDevice->flush();

    // Always truncate/create fresh file for YModem transfer
    // This ensures clean state for retries after failed transfers
    File f = fs->open(filepath, FILE_WRITE);
    if (!f) {
#ifdef YMODEM_LOGGING_ENABLED
        logYmodem("ERROR: File open failed");
#endif
        serialDevice->println("Error: Failed to open file for writing");
        return false;
    }

    // Y-modem receive state
    uint8_t *blockBuffer = (uint8_t *)malloc(128);
    if (!blockBuffer) {
        f.close();
#ifdef YMODEM_LOGGING_ENABLED
        logYmodem("ERROR: Memory allocation failed");
#endif
        serialDevice->println("Error: Failed to allocate memory");
        return false;
    }

#ifdef YMODEM_LOGGING_ENABLED
    logYmodem("YModem: Buffer allocated, ready for handshake");
#endif

    uint8_t expectedBlockNum = 1;
    uint32_t totalBytes = 0;
    uint32_t expectedFileSize = 0;
    bool firstBlock = true;
    bool headerReceived = false;

    // Clear any existing data in serial buffer and feed watchdog
    while (serialDevice->available()) {
        serialDevice->read();
        yield();
    }

    // Start Y-modem handshake - request CRC mode repeatedly
    unsigned long timeout = millis() + 60000; // 60 second timeout
    unsigned long lastCrcRequest = 0;
    unsigned long lastActivity = millis();
    unsigned long lastBlockTime = millis();
    bool hasReceivedData = false;
    uint16_t successfulBlocks = 0;
    uint32_t totalBlockTime = 0;

#ifdef YMODEM_LOGGING_ENABLED
    logYmodem("YModem: Starting handshake, will send CRC request every second...");
#endif

    while (millis() < timeout) {
        // Send CRC request every second until transfer starts
        if (!hasReceivedData && (millis() - lastCrcRequest) >= 1000) {
#ifdef YMODEM_LOGGING_ENABLED
            logYmodem("YModem: Sending CRC request...");
#endif
            writeByte(CRC);
            serialDevice->flush();
            lastCrcRequest = millis();
        }
        if (!serialDevice->available()) {
            // Sophisticated timeout logic
            unsigned long inactivityTime = millis() - lastActivity;
            unsigned long timeoutThreshold;

            if (successfulBlocks < 16) {
                // Early stage: use generous timeout for handshake and initial blocks
                timeoutThreshold = 15000; // 15 seconds
            } else {
                // Calculate adaptive timeout based on average block time
                uint32_t avgBlockTime = totalBlockTime / successfulBlocks;
                // Use 3x average block time, minimum 5 seconds, maximum 12 seconds
                timeoutThreshold = max(5000UL, min(12000UL, avgBlockTime * 3));
            }

            if (hasReceivedData && inactivityTime > timeoutThreshold) {
                // Adaptive inactivity timeout triggered
#ifdef YMODEM_LOGGING_ENABLED
                logYmodem(
                    "YModem: " + String(timeoutThreshold / 1000) + "s adaptive timeout after " +
                    String(successfulBlocks) + " blocks"
                );
#endif
                f.flush();
                f.close();
                free(blockBuffer);

                serialDevice->println("Y-modem transfer completed: " + String(totalBytes) + " bytes");
                serialDevice->println(
                    "(Transfer ended after " + String(timeoutThreshold / 1000) + " seconds of inactivity)"
                );
                serialDevice->println("Send file using Y-modem protocol (128-byte blocks only)");
                serialDevice->flush();

                // Clear any remaining serial data to prepare for next transfer
                delay(100);
                while (serialDevice->available()) {
                    serialDevice->read();
                    yield();
                }
                return true;
            }
            delay(10);
            continue;
        }

        lastActivity = millis(); // Update last activity time
        int header = serialDevice->read();

        if (header == EOT) {
            // End of transmission - normal completion only
#ifdef YMODEM_LOGGING_ENABLED
            logYmodem("YModem: Received EOT, transfer complete");
#endif
            writeByte(ACK);
            serialDevice->flush();
            f.flush(); // Ensure all data is written
            f.close();

            // Small delay to allow file system operations to complete
            delay(100);

            free(blockBuffer); // Clean up memory

            String completeMsg = "Y-modem transfer complete: " + String(totalBytes) + " bytes";
            if (expectedFileSize > 0) {
                if (totalBytes == expectedFileSize) {
                    completeMsg += " (matches expected size)";
                } else {
                    completeMsg += " (expected " + String(expectedFileSize) + " bytes)";
                }
            }
            serialDevice->println(completeMsg);
            serialDevice->flush();

            // Clear any remaining serial data to prepare for next transfer
            delay(100);
            while (serialDevice->available()) {
                serialDevice->read();
                yield();
            }

            return true;
        }

        if (header == CAN) {
            // Standard YModem cancellation - check for multiple CAN bytes for restart
#ifdef YMODEM_LOGGING_ENABLED
            logYmodem("YModem: Received CAN byte");
#endif

            // Look for additional CAN bytes to distinguish restart vs abort
            int canCount = 1;
            unsigned long canTimeout = millis() + 500; // 500ms to receive additional CAN bytes

            while (millis() < canTimeout && canCount < 3) {
                if (serialDevice->available()) {
                    int nextByte = serialDevice->read();
                    if (nextByte == CAN) {
                        canCount++;
#ifdef YMODEM_LOGGING_ENABLED
                        logYmodem("YModem: Additional CAN byte received (count: " + String(canCount) + ")");
#endif
                    } else {
                        // Not a CAN byte, put back the byte conceptually
                        break;
                    }
                } else {
                    delay(10);
                }
            }

            f.close();
            free(blockBuffer);

            if (canCount >= 2) {
                // Multiple CAN bytes = restart request
#ifdef YMODEM_LOGGING_ENABLED
                logYmodem("YModem: Multiple CAN bytes - restart requested");
#endif
                serialDevice->println("Y-modem transfer cancelled - ready for restart");
                serialDevice->println("Send file using Y-modem protocol (128-byte blocks only)");
            } else {
                // Single CAN = abort
#ifdef YMODEM_LOGGING_ENABLED
                logYmodem("YModem: Single CAN byte - transfer aborted");
#endif
                serialDevice->println("Y-modem transfer cancelled by sender");
                serialDevice->println("Send file using Y-modem protocol (128-byte blocks only)");
            }
            serialDevice->flush();

            // Complete state reset for restart
            delay(200);
            while (serialDevice->available()) {
                serialDevice->read();
                yield();
            }

            return false;
        }

        // Only accept SOH (128-byte blocks) for reliability
        if (header != SOH) {
#ifdef YMODEM_LOGGING_ENABLED
            logYmodem("YModem: Ignoring non-SOH header: 0x" + String(header, HEX));
#endif
            continue; // Reject STX and other headers
        }

        // Fixed 128-byte block size for maximum reliability
        size_t blockSize = 128;

        // Read block number and complement with timeout
        unsigned long readTimeout = millis() + 1000;
        while (!serialDevice->available() && millis() < readTimeout) delay(1);
        if (!serialDevice->available()) continue;
        uint8_t blockNumReceived = serialDevice->read();

        while (!serialDevice->available() && millis() < readTimeout) delay(1);
        if (!serialDevice->available()) continue;
        uint8_t blockNumComplement = serialDevice->read();

        // Verify block number
        if ((blockNumReceived + blockNumComplement) != 0xFF) {
            // Clear any remaining data in buffer before NAK
#ifdef YMODEM_LOGGING_ENABLED
            logYmodem(
                "ERROR: YModem: Bad block numbers: " + String(blockNumReceived) + "+" +
                String(blockNumComplement)
            );
#endif
            while (serialDevice->available()) {
                serialDevice->read();
                yield();
            }
            writeByte(NAK);
            serialDevice->flush();
            continue;
        }

        // Read data block
        size_t bytesRead = 0;
        unsigned long blockTimeout = millis() + 3000; // Reduced timeout for responsiveness

        while (bytesRead < blockSize && millis() < blockTimeout) {
            if (serialDevice->available()) {
                if (bytesRead < 128) { // Strict bounds check for 128-byte buffer
                    blockBuffer[bytesRead++] = serialDevice->read();
                } else {
                    // Buffer overflow protection - should never happen with 128-byte blocks
                    serialDevice->read(); // Consume byte but don't store
                    break;
                }
            } else {
                // Feed watchdog while waiting for data
                yield();
                delay(1);
            }
        }

        if (bytesRead != blockSize) {
#ifdef YMODEM_LOGGING_ENABLED
            logYmodem("ERROR: YModem: Block read timeout " + String(bytesRead) + "/" + String(blockSize));
#endif
            writeByte(NAK);
            serialDevice->flush();
            continue;
        }

        // Read CRC with timeout
        readTimeout = millis() + 1000;
        while (!serialDevice->available() && millis() < readTimeout) delay(1);
        if (!serialDevice->available()) {
            writeByte(NAK);
            serialDevice->flush();
            continue;
        }
        uint8_t crcHigh = serialDevice->read();

        while (!serialDevice->available() && millis() < readTimeout) delay(1);
        if (!serialDevice->available()) {
            writeByte(NAK);
            serialDevice->flush();
            continue;
        }
        uint8_t crcLow = serialDevice->read();

        uint16_t receivedCrc = (crcHigh << 8) | crcLow;
        uint16_t calculatedCrc = crc16(blockBuffer, blockSize);

        if (receivedCrc != calculatedCrc) {
            // Clear any remaining data in buffer before NAK
#ifdef YMODEM_LOGGING_ENABLED
            logYmodem(
                "ERROR: YModem: CRC error " + String(receivedCrc, HEX) + "!=" + String(calculatedCrc, HEX)
            );
#endif
            while (serialDevice->available()) {
                serialDevice->read();
                yield();
            }
            writeByte(NAK);
            serialDevice->flush();
            continue;
        }

        // Handle first block (may contain filename/size info in Y-modem)
        if (firstBlock && blockNumReceived == 0) {
            // Block 0 contains filename and size info - parse it
            // Format: filename\0size_as_string\0...
            char *filename = (char *)blockBuffer;
            char *sizeStr = filename + strlen(filename) + 1;

            if (strlen(sizeStr) > 0) { expectedFileSize = atol(sizeStr); }

#ifdef YMODEM_LOGGING_ENABLED
            logYmodem("YModem: Header block received, file size: " + String(expectedFileSize));
#endif
            writeByte(ACK);
            serialDevice->flush();
            firstBlock = false;
            headerReceived = true;
            expectedBlockNum = 1; // Next block should be 1
            continue;
        }

        // If this is the first block and no header was sent
        if (firstBlock && blockNumReceived == 1) {
            firstBlock = false;
            expectedBlockNum = 1;
        } else if (firstBlock) {
            // First block with non-standard numbering - sync to sender's numbering
            firstBlock = false;
            expectedBlockNum = blockNumReceived;
        }

        // Verify sequential block numbering
        if (blockNumReceived != (expectedBlockNum & 0xFF)) {
            if (blockNumReceived == ((expectedBlockNum - 1) & 0xFF)) {
                // Duplicate block - acknowledge but don't write
#ifdef YMODEM_LOGGING_ENABLED
                logYmodem("YModem: Duplicate block " + String(blockNumReceived));
#endif
                writeByte(ACK);
                serialDevice->flush();
                continue;
            } else {
                // Out of sequence - send NAK
#ifdef YMODEM_LOGGING_ENABLED
                logYmodem(
                    "ERROR: YModem: Sequence error " + String(blockNumReceived) +
                    "!=" + String(expectedBlockNum & 0xFF)
                );
#endif
                writeByte(NAK);
                serialDevice->flush();
                continue;
            }
        }

        // Write data to file, respecting expected file size or removing padding
        size_t bytesToWrite = blockSize;

        if (expectedFileSize > 0) {
            // We have file size from YModem header - use it to limit writes
            if (totalBytes >= expectedFileSize) {
                // File is already complete, don't write any more data
                bytesToWrite = 0;
            } else {
                size_t remainingBytes = expectedFileSize - totalBytes;
                if (remainingBytes < blockSize) {
                    // Only write the remaining bytes needed
                    bytesToWrite = remainingBytes;
                }
            }
        } else {
            // No file size available - fall back to padding removal
            bytesToWrite = removePadding(blockBuffer, blockSize);
        }

        if (bytesToWrite > 0) {
            size_t written = f.write(blockBuffer, bytesToWrite);
            if (written != bytesToWrite) {
                f.close();
                free(blockBuffer);
                writeByte(CAN);
                serialDevice->flush();

                serialDevice->println("Error: Failed to write block to file");
                return false;
            }
            totalBytes += bytesToWrite;
            hasReceivedData = true; // Mark that we've received actual data

            // Track timing for adaptive timeout
            unsigned long currentTime = millis();
            if (successfulBlocks > 0) { totalBlockTime += (currentTime - lastBlockTime); }
            lastBlockTime = currentTime;
            successfulBlocks++;

            // Show progress every 16 blocks
            if ((expectedBlockNum & 0x0F) == 0) {
#ifdef YMODEM_LOGGING_ENABLED
                uint32_t avgTime = successfulBlocks > 0 ? totalBlockTime / successfulBlocks : 0;
                logYmodem(
                    "YModem: Block " + String(expectedBlockNum) + ", " + String(totalBytes) +
                    " bytes (avg: " + String(avgTime) + "ms/block)"
                );
#endif
            }
        }
        expectedBlockNum++;

        // Acknowledge successful block
        writeByte(ACK);
        serialDevice->flush();

        // Reset timeout
        timeout = millis() + 60000;
        firstBlock = false;

        // Yield to other tasks to prevent watchdog timeout
        yield();
    }

    // Timeout occurred
#ifdef YMODEM_LOGGING_ENABLED
    logYmodem(
        "ERROR: YModem: Transfer timeout after " + String(totalBytes) + " bytes, " +
        String(successfulBlocks) + " blocks"
    );
#endif
    f.close();
    free(blockBuffer); // Clean up memory

    writeByte(CAN);
    serialDevice->println("Error: Y-modem receive timeout - ready for next transfer");
    serialDevice->println("Send file using Y-modem protocol (128-byte blocks only)");
    serialDevice->flush();

    // Clear serial buffer to prepare for next transfer
    delay(200);
    while (serialDevice->available()) {
        serialDevice->read();
        yield();
    }

    return false;
}

#endif
uint32_t renameCallback(cmd *c) {
    Command cmd(c);

    Argument filepathArg = cmd.getArgument("filepath");
    Argument newNameArg = cmd.getArgument("newName");
    String filepath = filepathArg.getValue();
    String newName = newNameArg.getValue();
    filepath.trim();
    newName.trim();

    if (filepath.length() == 0 || newName.length() == 0) return false;

    if (!filepath.startsWith("/")) filepath = "/" + filepath;
    if (!newName.startsWith("/")) newName = "/" + newName;

    FS *fs;
    if (!getFsStorage(fs)) return false;
    if (!(*fs).exists(filepath)) {
        serialDevice->println("File does not exist");
        return false;
    }

    if ((*fs).rename(filepath, newName)) {
        serialDevice->println("File renamed to '" + newName + "'");
        return true;
    }

    serialDevice->println("Error renaming file");
    return false;
}

uint32_t copyCallback(cmd *c) {
    Command cmd(c);

    Argument filepathArg = cmd.getArgument("filepath");
    Argument newNameArg = cmd.getArgument("newName");
    String filepath = filepathArg.getValue();
    String newName = newNameArg.getValue();
    filepath.trim();
    newName.trim();

    if (filepath.length() == 0 || newName.length() == 0) return false;

    if (!filepath.startsWith("/")) filepath = "/" + filepath;
    if (!newName.startsWith("/")) newName = "/" + newName;

    FS *fs;
    if (!getFsStorage(fs)) return false;
    if (!(*fs).exists(filepath)) {
        serialDevice->println("File does not exist");
        return false;
    }

    bool r;
    fileToCopy = filepath;
    if (pasteFile((*fs), newName)) {
        serialDevice->println("File copied to '" + newName + "'");
        r = true;
    } else {
        serialDevice->println("Error copying file");
        r = false;
    }

    fileToCopy = "";
    return r;
}

uint32_t mkdirCallback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument("filepath");
    String filepath = arg.getValue();
    filepath.trim();

    if (filepath.length() == 0) return false;

    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    FS *fs;
    if (!getFsStorage(fs)) return false;
    if ((*fs).exists(filepath)) {
        serialDevice->println("Directory already exists");
        return false;
    }

    if ((*fs).mkdir(filepath)) {
        serialDevice->println("Directory created");
        return true;
    }

    serialDevice->println("Error creating directory");
    return false;
}

uint32_t rmdirCallback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument("filepath");
    String filepath = arg.getValue();
    filepath.trim();

    if (filepath.length() == 0) return false;

    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    FS *fs;
    if (!getFsStorage(fs)) return false;
    if (!(*fs).exists(filepath)) {
        serialDevice->println("Directory does not exist");
        return false;
    }

    if ((*fs).rmdir(filepath)) {
        serialDevice->println("Directory removed");
        return true;
    }

    serialDevice->println("Error removing directory");
    return false;
}

uint32_t statCallback(cmd *c) {
    Command cmd(c);

    Argument arg = cmd.getArgument("filepath");
    String filepath = arg.getValue();
    filepath.trim();

    if (filepath.length() == 0) return false;

    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    FS *fs;
    if (!getFsStorage(fs) || !(*fs).exists(filepath)) return false;

    File file = fs->open(filepath);
    if (!file) return false;

    serialDevice->println("File: " + filepath);

    serialDevice->print("Size: ");
    serialDevice->print(file.size());
    serialDevice->print("\t\t");
    if (file.isDirectory()) serialDevice->print("directory");
    else serialDevice->print("regular file");
    serialDevice->println("");

    serialDevice->print("Modify: ");
    serialDevice->print(file.getLastWrite()); // TODO: parse to localtime
    serialDevice->println("");

    file.close();
    return true;
}

uint32_t freeStorageCallback(cmd *c) {
    Command cmd(c);
    Argument arg = cmd.getArgument("storage_type");

    if (arg.getValue() == "sd") {
        if (setupSdCard()) {
            uint64_t totalBytes = SD.totalBytes();
            uint64_t usedBytes = SD.usedBytes();
            uint64_t freeBytes = totalBytes - usedBytes;

            serialDevice->printf("SD Total space: %llu Bytes\n", totalBytes);
            serialDevice->printf("SD Used space: %llu Bytes\n", usedBytes);
            serialDevice->printf("SD Free space: %llu Bytes\n", freeBytes);
        } else {
            serialDevice->println("No SD card installed");
        }
    } else if (arg.getValue() == "littlefs") {

        uint64_t totalBytes = LittleFS.totalBytes();
        uint64_t usedBytes = LittleFS.usedBytes();
        uint64_t freeBytes = totalBytes - usedBytes;

        serialDevice->printf("LittleFS Total space: %llu Bytes\n", totalBytes);
        serialDevice->printf("LittleFS Used space: %llu Bytes\n", usedBytes);
        serialDevice->printf("LittleFS Free space: %llu Bytes\n", freeBytes);
    } else {
        serialDevice->printf("Invalid arg %s\n", arg.getValue().c_str());
        return false;
    }

    return true;
}

void createListCommand(SimpleCLI *cli) {
    Command cmd = cli->addCommand("ls,dir", listCallback);
    cmd.addPosArg("filepath", "");
}

void createReadCommand(SimpleCLI *cli) {
    Command cmd = cli->addCommand("cat,type", readCallback);
    cmd.addPosArg("filepath");
}

void createMd5Command(SimpleCLI *cli) {
    Command cmd = cli->addCommand("md5", md5Callback);
    cmd.addPosArg("filepath");
}

void createCrc32Command(SimpleCLI *cli) {
    // storage crc32
    Command cmd = cli->addCommand("crc32", crc32Callback);
    cmd.addPosArg("filepath");
}

void createRemoveCommand(SimpleCLI *cli) {
    Command cmd = cli->addCommand("rm,del", removeCallback);
    cmd.addPosArg("filepath");
}

void createMkdirCommand(SimpleCLI *cli) {
    Command cmd = cli->addCommand("md,mkdir", mkdirCallback);
    cmd.addPosArg("filepath");
}

void createRmdirCommand(SimpleCLI *cli) {
    Command cmd = cli->addCommand("rmdir", rmdirCallback);
    cmd.addPosArg("filepath");
}

void createStorageCommand(SimpleCLI *cli) {
    Command cmd = cli->addCompositeCommand("storage");

    Command cmdList = cmd.addCommand("list", listCallback);
    cmdList.addPosArg("filepath", "");

    Command cmdRead = cmd.addCommand("read", readCallback);
    cmdRead.addPosArg("filepath");

    Command cmdRemove = cmd.addCommand("remove", removeCallback);
    cmdRemove.addPosArg("filepath");

#ifndef LITE_VERSION
    Command cmdWrite = cmd.addCommand("write", writeCallback);
    cmdWrite.addPosArg("filepath");
    cmdWrite.addPosArg("size", "0");

    Command cmdYmodem = cmd.addCommand("ymodem", ymodemReceiveCallback);
    cmdYmodem.addPosArg("filepath");
#endif
    Command cmdRename = cmd.addCommand("rename", renameCallback);
    cmdRename.addPosArg("filepath");
    cmdRename.addPosArg("newName");

    Command cmdCopy = cmd.addCommand("copy", copyCallback);
    cmdCopy.addPosArg("filepath");
    cmdCopy.addPosArg("newName");

    Command cmdMkdir = cmd.addCommand("mkdir", mkdirCallback);
    cmdMkdir.addPosArg("filepath");

    Command cmdRmdir = cmd.addCommand("rmdir", rmdirCallback);
    cmdRmdir.addPosArg("filepath");

    Command cmdMd5 = cmd.addCommand("md5", md5Callback);
    cmdMd5.addPosArg("filepath");

    Command cmdCrc32 = cmd.addCommand("crc32", crc32Callback);
    cmdCrc32.addPosArg("filepath");

    Command cmdStat = cmd.addCommand("stat", statCallback);
    cmdStat.addPosArg("filepath");

    Command cmdFree = cmd.addCommand("free", freeStorageCallback);
    cmdFree.addPosArg("storage_type");
}

void createStorageCommands(SimpleCLI *cli) {
    createListCommand(cli);
    createReadCommand(cli);
    createRemoveCommand(cli);

    createMkdirCommand(cli);
    createRmdirCommand(cli);

    createMd5Command(cli);
    createCrc32Command(cli);

    createStorageCommand(cli);
}
