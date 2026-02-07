#include "helpers.h"
#ifndef LITE_VERSION
#include <globals.h>

bool _setupPsramFs() {
    // https://github.com/tobozo/ESP32-PsRamFS/blob/main/examples/PSRamFS_Test/PSRamFS_Test.ino
    static bool psRamFSMounted = false;
    if (psRamFSMounted) return true; // avoid reinit

#ifdef BOARD_HAS_PSRAM
    PSRamFS.setPartitionSize(ESP.getFreePsram() / 2); // use half of psram
#else
    PSRamFS.setPartitionSize(SAFE_STACK_BUFFER_SIZE);
#endif

    if (!PSRamFS.begin()) {
        serialDevice->println("PSRamFS Mount Failed");
        psRamFSMounted = false;
        return false;
    }
    // else
    psRamFSMounted = true;
    return true;
}

char *_readFileFromSerial(size_t fileSizeChar) {
    char *buf = psramFound() ? (char *)ps_malloc(fileSizeChar + 1) : (char *)malloc(fileSizeChar + 1);

    if (buf == NULL) {
        serialDevice->printf("Could not allocate %d\n", fileSizeChar);
        return NULL;
    }

    size_t bufSize = 0;
    buf[0] = '\0';

    unsigned long lastData = millis();

    String currLine = "";
    serialDevice->println("Reading input data from serial buffer until EOF");
    serialDevice->flush();
    while (true) {
        if (!serialDevice->available()) {
            if (millis() - lastData > 5000) break; // timeout
            delay(10);
            continue;
        }

        lastData = millis();
        currLine = serialDevice->readStringUntil('\n');
        if (currLine == "EOF") break;
        size_t lineLength = currLine.length();

        if (bufSize + lineLength + 2 > fileSizeChar) {
            log_e("Input truncated!");
            break;
        }

        memcpy(buf + bufSize, currLine.c_str(), lineLength);
        bufSize += lineLength;
        buf[bufSize++] = '\n';
    }
    buf[bufSize] = '\0';
    return buf;
}
#endif
